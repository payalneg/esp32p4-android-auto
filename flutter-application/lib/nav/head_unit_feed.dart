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

/// A tile the head unit refused this many times is not tried again.
const int kTileMaxAttempts = 2;

/// How many queued tiles one pass may skip past before giving the position
/// its turn again. Keeps a run of uncached tiles from stalling the feed
/// without letting a pass run long.
const int kSkipBudget = 8;

/// Zoom the head unit composes at. 17 puts about 600 m across the panel at
/// Kraków's latitude, which is a town-scale view at riding speed; the phone
/// already prefetches 18 and 19 along a route, and 17 sits one step out.
const int kHeadUnitZoom = 17;

/// The fallback layers, in levels below the detail zoom. Must match
/// NAV_COARSE_DZ and NAV_WIDE_DZ in the firmware.
///
/// Three levels down bridges the seconds while sharp tiles arrive. Six levels
/// down is twenty kilometres a tile, so a couple of them cover a whole city —
/// which is what stops the screen going bare when the view moves somewhere
/// nothing has been cached.
const int kCoarseDz = 3;
const int kWideDz = 6;
const int kCoarseZoom = kHeadUnitZoom - kCoarseDz;
const int kWideZoom = kHeadUnitZoom - kWideDz;

/// Slippy tiles are 256 px square.
const int kTilePx = 256;

/// A ring of tiles beyond the screen, so the map does not run out at the edge
/// the moment the rider moves.
const int kTileMargin = 1;

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

  Future<void> sendView(double lat, double lon, int zoom, int headingDeg,
      {double speedMs});
}

/// Pushes tiles and positions to the head unit for as long as it is looking.
class HeadUnitFeed {
  HeadUnitFeed({
    /// Read on each pass rather than held: the cache only exists once the map
    /// data has been opened, which may be after this screen is built.
    required TileCache? Function() tiles,
    required HeadUnitLink link,
    Future<void> Function(Duration)? sleep,
    DateTime Function()? now,
  })  : _tiles = tiles,
        _link = link,
        _now = now ?? DateTime.now,
        _sleep = sleep ?? _realSleep {
    _stateSub = _link.displayStates.listen((st) {
      // A head unit that went away and came back has an empty tile store.
      if (!st.visible) _sentThisSession.clear();
    });
  }

  final TileCache? Function() _tiles;
  final HeadUnitLink _link;
  final Future<void> Function(Duration) _sleep;
  final DateTime Function() _now;
  late final StreamSubscription<NavDisplayState> _stateSub;

  final status = ValueNotifier<HeadUnitFeedStatus>(const HeadUnitFeedStatus());

  /// Tiles this connection has already accepted — the head unit keeps them, so
  /// they must never be sent twice.
  final _sentThisSession = <TileId>{};
  final _attempts = <TileId, int>{};

  LatLon? _where;
  double? _headingDeg;
  double _speedMs = 0;
  DateTime? _lastViewAt;
  bool _running = false;
  int _tilesSent = 0;
  int _tilesFailed = 0;
  int _viewsSent = 0;
  int _lastTileBytes = 0;

  static Future<void> _realSleep(Duration d) => Future<void>.delayed(d);

  bool get running => _running;

  /// The rider moved. Cheap to call on every fix; the loop picks up the latest.
  void setPosition(LatLon at, {double? headingDeg, double? speedMs}) {
    _where = at;
    if (headingDeg != null) _headingDeg = headingDeg;
    if (speedMs != null) _speedMs = speedMs;
  }

  void start() {
    if (_running) return;
    _running = true;
    unawaited(_loop());
  }

  Future<void> stop() async {
    _running = false;
    _lastViewAt = null;
    _speedMs = 0;
    _publish();
  }

  Future<void> dispose() async {
    await stop();
    await _stateSub.cancel();
    status.dispose();
  }

  Future<void> _loop() async {
    while (_running) {
      final sentTile = await _tick();
      if (!_running) return;
      // A tile in flight means the map is still filling: go straight on to the
      // next one. Pausing a view period between tiles was nearly half the time
      // a screenful took, and while it fills there is no new position to
      // report anyway — the rider is looking at an empty display.
      await _sleep(sentTile ? Duration.zero : _untilNextView());
    }
  }

  Duration _untilNextView() {
    final last = _lastViewAt;
    if (last == null) return Duration.zero;
    final due = kViewPeriod - _now().difference(last);
    return due > Duration.zero ? due : Duration.zero;
  }

  /// One pass: say where the rider is if that is due, then hand the head unit
  /// one tile it is missing. Returns whether a tile went out. Public for
  /// tests, which drive it directly.
  @visibleForTesting
  Future<bool> tick() => _tick();

  Future<bool> _tick() async {
    final at = _where;
    if (!_link.available || !_link.displayState.visible || at == null) {
      _publish();
      return false;
    }

    if (_untilNextView() == Duration.zero) {
      await _link.sendView(at.lat, at.lon, kHeadUnitZoom,
          (_headingDeg ?? 0).round() % 360,
          speedMs: _speedMs);
      _lastViewAt = _now();
      _viewsSent++;
    }

    final missing = _missingTiles(at);
    _publish(pending: missing.length);
    if (missing.isEmpty) return false;

    // Walk the queue until one tile actually goes out. A tile that is neither
    // cached nor reachable must not cost a whole pass — with no signal the
    // feed would otherwise stall on it while the head unit waits for ground
    // it could have had.
    for (final t in missing.take(kSkipBudget)) {
      if (await _sendTile(t)) return true;
    }
    return false;
  }

  /// Exactly the tiles the head unit's screen covers around [at], plus a ring,
  /// nearest to the middle first.
  ///
  /// Not a disc and not a square of rings: the panel is 800x480, so it reaches
  /// two tiles sideways but only one up and down. Asking in rings sent the
  /// off-screen corners before the tiles either side of the rider, and the map
  /// stayed half empty while the link was busy with ground nobody could see.
  List<TileId> viewportTiles(LatLon at) {
    final n = 1 << kHeadUnitZoom;
    final centre = deg2tileF(at.lat, at.lon, kHeadUnitZoom);
    final halfW = kHeadUnitW / 2 / kTilePx;
    final halfH = kHeadUnitH / 2 / kTilePx;
    final x0 = (centre.x - halfW).floor() - kTileMargin;
    final x1 = (centre.x + halfW).floor() + kTileMargin;
    final y0 = (centre.y - halfH).floor() - kTileMargin;
    final y1 = (centre.y + halfH).floor() + kTileMargin;

    // What the panel actually covers, before the ring is added.
    final sx0 = (centre.x - halfW).floor();
    final sx1 = (centre.x + halfW).floor();
    final sy0 = (centre.y - halfH).floor();
    final sy1 = (centre.y + halfH).floor();

    final out = <({TileId tile, int ring, double d})>[];
    for (var y = y0; y <= y1; y++) {
      if (y < 0 || y >= n) continue;
      for (var x = x0; x <= x1; x++) {
        // The world wraps sideways; the head unit does the same arithmetic.
        var wx = x % n;
        if (wx < 0) wx += n;
        final dx = (x + 0.5) - centre.x;
        final dy = (y + 0.5) - centre.y;
        // Anything the rider can see outranks anything they cannot, however
        // close: a corner of the screen matters more than the ground just
        // above it.
        final visible = x >= sx0 && x <= sx1 && y >= sy0 && y <= sy1;
        out.add((
          tile: TileId(kHeadUnitZoom, wx, y),
          ring: visible ? 0 : 1,
          d: dx * dx + dy * dy,
        ));
      }
    }
    out.sort((a, b) =>
        a.ring != b.ring ? a.ring - b.ring : a.d.compareTo(b.d));
    return <TileId>[for (final e in out) e.tile];
  }

  /// Fallback tiles for the ground the panel is showing, coarsest first: one
  /// wide tile, then the handful of middling ones. These are what the head
  /// unit draws under everything, so they go before anything sharp — a
  /// blurred map beats a bare one.
  List<TileId> fallbackTiles(LatLon at) => <TileId>[
        ..._layerTiles(at, kWideZoom, 0),
        ..._layerTiles(at, kCoarseZoom, 0),
      ];

  /// The ring of wide tiles around the panel. A safety net for movement —
  /// twenty kilometres a tile, so this is what keeps the map from running out
  /// when the rider covers ground faster than detail tiles arrive. Sent last,
  /// because none of it is on screen yet.
  List<TileId> wideRingTiles(LatLon at) {
    final centre = _layerTiles(at, kWideZoom, 0).toSet();
    return <TileId>[
      for (final t in _layerTiles(at, kWideZoom, 1))
        if (!centre.contains(t)) t,
    ];
  }

  /// Tiles of one zoom covering the panel, plus `ring` tiles around it,
  /// nearest the middle first.
  List<TileId> _layerTiles(LatLon at, int zoom, int ring) {
    final n = 1 << zoom;
    final centre = deg2tileF(at.lat, at.lon, zoom);
    final scale = 1 << (kHeadUnitZoom - zoom);
    final halfW = kHeadUnitW / 2 / kTilePx / scale;
    final halfH = kHeadUnitH / 2 / kTilePx / scale;
    final ranked = <({TileId tile, double d})>[];
    for (var y = (centre.y - halfH).floor() - ring;
        y <= (centre.y + halfH).floor() + ring;
        y++) {
      if (y < 0 || y >= n) continue;
      for (var x = (centre.x - halfW).floor() - ring;
          x <= (centre.x + halfW).floor() + ring;
          x++) {
        var wx = x % n;
        if (wx < 0) wx += n;
        final dx = (x + 0.5) - centre.x;
        final dy = (y + 0.5) - centre.y;
        ranked.add((tile: TileId(zoom, wx, y), d: dx * dx + dy * dy));
      }
    }
    ranked.sort((a, b) => a.d.compareTo(b.d));
    return <TileId>[for (final e in ranked) e.tile];
  }

  List<TileId> _missingTiles(LatLon at) {
    bool needed(TileId t) =>
        !_sentThisSession.contains(t) &&
        (_attempts[t] ?? 0) < kTileMaxAttempts;
    return <TileId>[
      for (final t in fallbackTiles(at))
        if (needed(t)) t,
      for (final t in viewportTiles(at))
        if (needed(t)) t,
      for (final t in wideRingTiles(at))
        if (needed(t)) t,
    ];
  }

  /// Returns whether the tile reached the head unit.
  Future<bool> _sendTile(TileId t) async {
    // The head unit's view is not always where the rider's own map is looking,
    // so a tile it needs may never have been downloaded. Fetch it here rather
    // than hope the map screen wanders over it — as bulk, so it queues behind
    // whatever the rider is actually looking at.
    final cache = _tiles();
    if (cache == null) return false;
    final bytes = await cache.read(t) ??
        await cache.fetchAndStore(t, tileUrls(t), priority: TilePriority.bulk);
    if (bytes == null) {
      // No tile and no network. Count the attempt so one that never arrives
      // stops being asked for on every pass.
      _attempts[t] = (_attempts[t] ?? 0) + 1;
      return false;
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
    return r.ok;
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
