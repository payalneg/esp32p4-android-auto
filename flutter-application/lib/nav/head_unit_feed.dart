/// Feeding the head unit's own map: the tiles it needs, then where the rider is.
///
/// The phone already downloads and caches raster tiles for offline use. Rather
/// than render a picture and send it again for every few pixels of movement,
/// this forwards the tiles themselves — each one once — and then says only
/// where the rider is, a dozen bytes at a time. The head unit draws the map
/// from what it holds, as often as it likes.
///
/// That also means nothing here needs a widget, a canvas or a visible screen,
/// so it keeps working with the phone face-down in a pocket.
library;

import 'dart:async';
import 'dart:math' as math;

import 'package:flutter/foundation.dart';

import '../ble/nav_stream.dart';
import 'geo.dart';
import 'tile_cache.dart';
import 'tile_math.dart';

/// The head unit's panel, in pixels — what a screenful of tiles has to cover.
const int kHeadUnitW = 800;
const int kHeadUnitH = 480;

/// How often the rider's position goes out. The head unit redraws on each one,
/// so this is the map's frame rate; twice a second looks alive without costing
/// anything worth measuring (13 bytes a time).
const Duration kViewPeriod = Duration(milliseconds: 500);

/// Tiles are pushed one at a time between position updates. This is the pause
/// between them — enough to keep the link responsive for the small messages.
const Duration kTileGap = Duration(milliseconds: 60);

/// A tile the head unit refused this many times is not tried again.
const int kTileMaxAttempts = 2;

/// Zoom the head unit composes at. 17 puts about 600 m across the panel at
/// Kraków's latitude, which is a town-scale view at riding speed; the phone
/// already prefetches 18 and 19 along a route, and 17 sits one step out.
const int kHeadUnitZoom = 17;

/// What the feed is doing, for the screen that shows it.
@immutable
class HeadUnitFeedStatus {
  const HeadUnitFeedStatus({
    this.tilesSent = 0,
    this.tilesPending = 0,
    this.tilesFailed = 0,
    this.viewsSent = 0,
    this.lastTileBytes = 0,
    this.running = false,
  });

  final int tilesSent;
  final int tilesPending;
  final int tilesFailed;
  final int viewsSent;
  final int lastTileBytes;
  final bool running;

  /// Everything the current view needs is over there.
  bool get complete => running && tilesPending == 0;

  @override
  bool operator ==(Object other) =>
      other is HeadUnitFeedStatus &&
      other.tilesSent == tilesSent &&
      other.tilesPending == tilesPending &&
      other.tilesFailed == tilesFailed &&
      other.viewsSent == viewsSent &&
      other.lastTileBytes == lastTileBytes &&
      other.running == running;

  @override
  int get hashCode => Object.hash(
      tilesSent, tilesPending, tilesFailed, viewsSent, lastTileBytes, running);
}

/// What the feed needs from the link. Narrow on purpose, so the loop can be
/// driven by a fake in tests.
abstract class HeadUnitLink {
  /// A head unit is connected and its firmware understands the tile protocol.
  bool get available;

  /// Whether its navigator screen is the one being looked at.
  NavDisplayState get displayState;
  Stream<NavDisplayState> get displayStates;

  Future<NavFrameResult> sendTile(
      int z, int x, int y, int format, Uint8List bytes);

  Future<void> sendView(
      double lat, double lon, int zoom, int headingDeg);
}

/// Pushes tiles and positions to the head unit for as long as it is looking.
class HeadUnitFeed {
  HeadUnitFeed({
    /// Read on each pass rather than held: the cache only exists once the map
    /// data has been opened, which may be after this screen is built.
    required TileCache? Function() tiles,
    required HeadUnitLink link,
    Future<void> Function(Duration)? sleep,
  })  : _tiles = tiles,
        _link = link,
        _sleep = sleep ?? _realSleep {
    _stateSub = _link.displayStates.listen((st) {
      // A head unit that went away and came back has an empty tile store.
      if (!st.visible) _sentThisSession.clear();
    });
  }

  final TileCache? Function() _tiles;
  final HeadUnitLink _link;
  final Future<void> Function(Duration) _sleep;
  late final StreamSubscription<NavDisplayState> _stateSub;

  final status = ValueNotifier<HeadUnitFeedStatus>(const HeadUnitFeedStatus());

  /// Tiles this connection has already accepted — the head unit keeps them, so
  /// they must never be sent twice.
  final _sentThisSession = <TileId>{};
  final _attempts = <TileId, int>{};

  LatLon? _where;
  double? _headingDeg;
  bool _running = false;
  int _tilesSent = 0;
  int _tilesFailed = 0;
  int _viewsSent = 0;
  int _lastTileBytes = 0;

  static Future<void> _realSleep(Duration d) => Future<void>.delayed(d);

  bool get running => _running;

  /// The rider moved. Cheap to call on every fix; the loop picks up the latest.
  void setPosition(LatLon at, {double? headingDeg}) {
    _where = at;
    if (headingDeg != null) _headingDeg = headingDeg;
  }

  void start() {
    if (_running) return;
    _running = true;
    unawaited(_loop());
  }

  Future<void> stop() async {
    _running = false;
    _publish();
  }

  Future<void> dispose() async {
    await stop();
    await _stateSub.cancel();
    status.dispose();
  }

  Future<void> _loop() async {
    while (_running) {
      final ok = await _tick();
      if (!_running) return;
      await _sleep(ok ? kViewPeriod : const Duration(seconds: 1));
    }
  }

  /// One pass: tell the head unit where we are, then hand it one tile it is
  /// missing. Public for tests, which drive it directly.
  @visibleForTesting
  Future<bool> tick() => _tick();

  Future<bool> _tick() async {
    final at = _where;
    if (!_link.available || !_link.displayState.visible || at == null) {
      _publish();
      return false;
    }

    await _link.sendView(at.lat, at.lon, kHeadUnitZoom,
        (_headingDeg ?? 0).round() % 360);
    _viewsSent++;

    final missing = _missingTiles(at);
    _publish(pending: missing.length);
    if (missing.isEmpty) return true;

    // One tile per pass: a screenful is a minute of link time, and the rider's
    // position has to keep flowing while it fills.
    await _sendTile(missing.first);
    await _sleep(kTileGap);
    return true;
  }

  /// Tiles covering the head unit's screen around [at], nearest first, that it
  /// does not already have.
  List<TileId> _missingTiles(LatLon at) {
    // A screenful plus a ring, so the map does not run out at the edge the
    // moment the rider moves.
    const radiusPx = kHeadUnitW / 2 + kHeadUnitH / 2;
    final metresPerPixel = 156543.03392 *
        math.cos(at.lat * math.pi / 180.0) /
        (1 << kHeadUnitZoom);
    final wanted = tilesAround(
      at,
      radiusM: radiusPx * metresPerPixel,
      zooms: const <int>[kHeadUnitZoom],
      maxTiles: 64,
    );
    return <TileId>[
      for (final t in wanted)
        if (!_sentThisSession.contains(t) &&
            (_attempts[t] ?? 0) < kTileMaxAttempts)
          t,
    ];
  }

  Future<void> _sendTile(TileId t) async {
    // The head unit's view is not always where the rider's own map is looking,
    // so a tile it needs may never have been downloaded. Fetch it here rather
    // than hope the map screen wanders over it — as bulk, so it queues behind
    // whatever the rider is actually looking at.
    final cache = _tiles();
    if (cache == null) return;
    final bytes = await cache.read(t) ??
        await cache.fetchAndStore(t, tileUrls(t), priority: TilePriority.bulk);
    if (bytes == null) {
      // No tile and no network. Count the attempt so one that never arrives
      // stops being asked for on every pass.
      _attempts[t] = (_attempts[t] ?? 0) + 1;
      return;
    }
    final r = await _link.sendTile(t.z, t.x, t.y, kTileFormatPng, bytes);
    if (r.ok) {
      _sentThisSession.add(t);
      _tilesSent++;
      _lastTileBytes = bytes.length;
    } else {
      _tilesFailed++;
      _attempts[t] = (_attempts[t] ?? 0) + 1;
    }
    _publish();
  }

  void _publish({int? pending}) {
    final next = HeadUnitFeedStatus(
      tilesSent: _tilesSent,
      tilesPending: pending ?? status.value.tilesPending,
      tilesFailed: _tilesFailed,
      viewsSent: _viewsSent,
      lastTileBytes: _lastTileBytes,
      running: _running,
    );
    if (status.value != next) status.value = next;
  }
}
