/// Feeding the head unit its own map: which tiles go over, and how often the
/// rider's position does.
library;

import 'dart:async';
import 'dart:io';
import 'dart:typed_data';

import 'package:aa_bridge/ble/nav_stream.dart';
import 'package:aa_bridge/nav/geo.dart';
import 'package:aa_bridge/nav/head_unit_feed.dart';
import 'package:aa_bridge/nav/tile_cache.dart';
import 'package:aa_bridge/nav/tile_math.dart';
import 'package:flutter_test/flutter_test.dart';

class _FakeLink implements HeadUnitLink {
  @override
  bool available = true;

  NavDisplayState _state =
      const NavDisplayState(navMode: true, visible: true, maxChunk: 509);
  final _states = StreamController<NavDisplayState>.broadcast();
  final _dropped = StreamController<({int z, int x, int y})>.broadcast();
  final _emptied = StreamController<void>.broadcast();

  final tiles = <TileId>[];
  final views = <({double lat, double lon, int zoom, int heading})>[];
  int ack = NavAck.ok;

  @override
  NavDisplayState get displayState => _state;

  @override
  Stream<NavDisplayState> get displayStates => _states.stream;

  @override
  Stream<({int z, int x, int y})> get dropped => _dropped.stream;

  @override
  Stream<void> get emptied => _emptied.stream;

  /// The head unit says it is holding nothing at all.
  void empty() => _emptied.add(null);

  void drop(TileId t) => _dropped.add((z: t.z, x: t.x, y: t.y));

  void setVisible(bool v) {
    _state = NavDisplayState(navMode: v, visible: v, maxChunk: 509);
    _states.add(_state);
  }

  @override
  Future<NavFrameResult> sendTile(
      int z, int x, int y, int format, Uint8List bytes) async {
    tiles.add(TileId(z, x, y));
    return NavFrameResult(ack, 0, 3);
  }

  @override
  Future<void> sendView(double lat, double lon, int zoom, int heading,
      {double speedMs = 0}) async {
    views.add((lat: lat, lon: lon, zoom: zoom, heading: heading));
  }

  final routes = <int>[];      // point counts
  final guides = <int>[];      // turn kinds

  @override
  Future<NavFrameResult> sendRoute(List<({double lat, double lon})> pts) async {
    routes.add(pts.length);
    return NavFrameResult(ack, 0, 1);
  }

  @override
  Future<void> sendGuide({
    required int turn,
    required int distM,
    required int remainingM,
    required int remainingS,
    required bool offRoute,
  }) async {
    guides.add(turn);
  }

  Future<void> close() async {
    await _states.close();
    await _dropped.close();
    await _emptied.close();
  }
}

/// A tile cache with no network: whatever was seeded is all there is.
TileCache _offlineCache(Directory root) =>
    TileCache(root, userAgent: 'test/1.0', clientFactory: () => throw 'offline');

const _krakow = LatLon(50.0619, 19.9368);

void main() {
  late Directory root;
  late TileCache cache;
  late _FakeLink link;

  /// Put a tile on disk so the feed has something to forward.
  Future<void> seed(TileId t) async {
    final f = cache.fileFor(t);
    await f.parent.create(recursive: true);
    await f.writeAsBytes(Uint8List.fromList(List<int>.filled(64, 7)));
  }

  /// The blurred fallback tile for the same spot. It is sent before anything
  /// sharp, so most tests need it on disk too.
  Future<TileId> seedCoarse() async {
    // The widest layer goes first, so that is the one a single pass sends.
    final t = deg2tile(_krakow.lat, _krakow.lon, kWideZoom);
    await seed(t);
    return t;
  }

  HeadUnitFeed build() => HeadUnitFeed(
        tiles: () => cache,
        link: link,
        sleep: (_) async {},
      );

  setUp(() {
    root = Directory.systemTemp.createTempSync('hufeed');
    cache = _offlineCache(root);
    link = _FakeLink();
  });

  tearDown(() async {
    await link.close();
    if (root.existsSync()) root.deleteSync(recursive: true);
  });

  test('nothing goes out before the rider has a position', () async {
    final feed = build();
    expect(await feed.tick(), isFalse);
    expect(link.views, isEmpty);
    expect(link.tiles, isEmpty);
  });

  test('nothing goes out while the head unit shows another screen', () async {
    link.setVisible(false);
    final feed = build()..setPosition(_krakow);
    expect(await feed.tick(), isFalse);
    expect(link.views, isEmpty);
  });

  test('every pass says where the rider is', () async {
    final feed = build()..setPosition(_krakow, headingDeg: 91.4);
    await feed.tick();
    expect(link.views.length, 1);
    expect(link.views.first.lat, closeTo(_krakow.lat, 1e-9));
    expect(link.views.first.lon, closeTo(_krakow.lon, 1e-9));
    expect(link.views.first.zoom, kHeadUnitZoom);
    expect(link.views.first.heading, 91);
  });

  test('the blurred layers go first, sharpest last', () async {
    // Moving faster than 25 KB tiles can arrive must show a blurred map, not
    // a hole — so the fallback layers cross the link first, widest of all
    // first, and the detail tiles come behind them.
    final wide = deg2tile(_krakow.lat, _krakow.lon, kWideZoom);
    final coarse = deg2tile(_krakow.lat, _krakow.lon, kCoarseZoom);
    final here = deg2tile(_krakow.lat, _krakow.lon, kHeadUnitZoom);
    for (final t in <TileId>[wide, coarse, here]) {
      await seed(t);
    }
    final feed = build()..setPosition(_krakow);

    // The widest layer asks for a ring the offline cache cannot supply, so a
    // few passes go by dropping those before the finer layers come up.
    for (var i = 0; i < 10; i++) {
      await feed.tick();
    }
    expect(link.tiles.first, wide);
    expect(link.tiles, containsAllInOrder(<TileId>[wide, coarse, here]));
    expect(feed.status.value.tilesSent, 3);

    // Everything else around is missing from the cache and unreachable, so
    // none of the three goes again.
    for (var i = 0; i < 5; i++) {
      await feed.tick();
    }
    for (final t in <TileId>[wide, coarse, here]) {
      expect(link.tiles.where((x) => x == t).length, 1);
    }
  });

  test('a refused tile is retried, then given up on', () async {
    await seedCoarse();
    link.ack = NavAck.decodeFailed;
    final feed = build()..setPosition(_krakow);

    for (var i = 0; i < 5; i++) {
      await feed.tick();
    }
    // Tried, but not for ever: a tile the head unit cannot take is dropped
    // rather than blocking the queue behind it on every pass.
    expect(link.tiles.length, kTileMaxAttempts);
    expect(feed.status.value.tilesFailed, kTileMaxAttempts);
  });

  test('a head unit that went away is told everything again', () async {
    final here = await seedCoarse();
    final feed = build()..setPosition(_krakow);
    await feed.tick();
    expect(link.tiles.length, 1);

    // Its store is in RAM: a reconnect starts from nothing.
    link.setVisible(false);
    await Future<void>.delayed(Duration.zero);
    link.setVisible(true);
    await Future<void>.delayed(Duration.zero);

    await feed.tick();
    expect(link.tiles.length, 2);
    expect(link.tiles.last, here);
  });

  test('the tiles asked for are the ones the panel covers', () {
    final feed = build();
    final tiles = feed.viewportTiles(_krakow);
    final here = deg2tile(_krakow.lat, _krakow.lon, kHeadUnitZoom);

    // The tile under the rider first — the one the screen most obviously
    // lacks — and a screenful plus a ring in total.
    expect(tiles.first, here);
    expect(tiles.length, inInclusiveRange(24, 42));
    expect(tiles.toSet().length, tiles.length, reason: 'no duplicates');

    // The panel is 800x480: it reaches two tiles sideways but only one up and
    // down. Everything actually on screen must be sent before any of the ring
    // around it — asking in square rings put off-screen corners first and the
    // map stayed half empty.
    final centre = deg2tileF(_krakow.lat, _krakow.lon, kHeadUnitZoom);
    bool onScreen(TileId t) =>
        t.x >= (centre.x - kHeadUnitW / 2 / kTilePx).floor() &&
        t.x <= (centre.x + kHeadUnitW / 2 / kTilePx).floor() &&
        t.y >= (centre.y - kHeadUnitH / 2 / kTilePx).floor() &&
        t.y <= (centre.y + kHeadUnitH / 2 / kTilePx).floor();
    final lastOnScreen = tiles.lastIndexWhere(onScreen);
    final firstOffScreen = tiles.indexWhere((t) => !onScreen(t));
    expect(firstOffScreen, greaterThan(lastOnScreen),
        reason: 'a margin tile was queued ahead of one the rider can see');
  });

  test('while the map is filling, tiles do not wait on position updates',
      () async {
    // A screenful used to take a view period per tile on top of the transfer,
    // which was nearly half the time it took to fill.
    await seedCoarse();
    final fine = deg2tile(_krakow.lat, _krakow.lon, kHeadUnitZoom);
    var clock = DateTime(2026);
    final feed = HeadUnitFeed(
      tiles: () => cache,
      link: link,
      sleep: (_) async {},
      now: () => clock,
    )..setPosition(_krakow);

    // First pass: the position is due, and a tile goes with it.
    expect(await feed.tick(), isTrue);
    expect(link.views.length, 1);
    expect(link.tiles.length, 1);

    // A moment later, still inside the view period: the next tile goes anyway
    // and no second position is sent.
    clock = clock.add(const Duration(milliseconds: 50));
    await seed(fine);
    expect(await feed.tick(), isTrue);
    expect(link.views.length, 1);
    expect(link.tiles.length, 2);

    // Once the period has passed, the position goes out again.
    clock = clock.add(kViewPeriod);
    await seed(TileId(fine.z, fine.x + 1, fine.y));
    await feed.tick();
    expect(link.views.length, 2);
  });

  test('a tile the head unit dropped is sent again', () async {
    // Its store is in RAM and finite. The feed never repeats itself, so an
    // eviction it does not hear about is ground that stays blurred for ever.
    final wide = await seedCoarse();
    final feed = build()..setPosition(_krakow);
    await feed.tick();
    expect(link.tiles, <TileId>[wide]);

    // Sent once and not again...
    await feed.tick();
    expect(link.tiles.where((t) => t == wide).length, 1);

    // ...until the head unit says it had to let it go.
    link.drop(wide);
    await Future<void>.delayed(Duration.zero);
    await feed.tick();
    expect(link.tiles.where((t) => t == wide).length, 2);
  });

  test('a head unit whose store emptied is sent everything again', () async {
    // A reboot while its navigator screen stays up: the visibility never
    // changes, so this notice is the only signal that the store is gone.
    final wide = await seedCoarse();
    final feed = build()..setPosition(_krakow);
    await feed.tick();
    expect(link.tiles, <TileId>[wide]);

    await feed.tick();
    expect(link.tiles.where((t) => t == wide).length, 1, reason: 'sent once');

    link.empty();
    await Future<void>.delayed(Duration.zero);
    await feed.tick();
    expect(link.tiles.where((t) => t == wide).length, 2);
  });

  test('the line and the turn go before any tile', () async {
    // A rider needs to know where to go more than they need sharp ground.
    await seedCoarse();
    final feed = build()..setPosition(_krakow);
    feed.setRoute('route-1', <({double lat, double lon})>[
      (lat: 50.06, lon: 19.93),
      (lat: 50.07, lon: 19.94),
    ]);
    feed.setGuide(
        turn: 3, distM: 80, remainingM: 700, remainingS: 120, offRoute: false);

    await feed.tick();
    expect(link.routes, <int>[2]);
    expect(link.tiles, isEmpty, reason: 'the line came first');

    await feed.tick();
    expect(link.guides, <int>[3]);

    // Neither repeats itself.
    await feed.tick();
    expect(link.routes.length, 1);
    expect(link.guides.length, 1);
    expect(link.tiles.length, 1);
  });

  test('stopping ends the loop', () async {
    final feed = build()..setPosition(_krakow);
    feed.start();
    expect(feed.running, isTrue);
    await feed.stop();
    expect(feed.running, isFalse);
  });
}
