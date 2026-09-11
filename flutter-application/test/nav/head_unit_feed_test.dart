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

  final tiles = <TileId>[];
  final views = <({double lat, double lon, int zoom, int heading})>[];
  int ack = NavAck.ok;

  @override
  NavDisplayState get displayState => _state;

  @override
  Stream<NavDisplayState> get displayStates => _states.stream;

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
  Future<void> sendView(double lat, double lon, int zoom, int heading) async {
    views.add((lat: lat, lon: lon, zoom: zoom, heading: heading));
  }

  Future<void> close() => _states.close();
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

  test('the tile under the rider goes over, and only once', () async {
    final here = deg2tile(_krakow.lat, _krakow.lon, kHeadUnitZoom);
    await seed(here);
    final feed = build()..setPosition(_krakow);

    // One tile per pass, so the position keeps flowing while the map fills.
    await feed.tick();
    expect(link.tiles, <TileId>[here]);
    expect(feed.status.value.tilesSent, 1);

    // Everything else around is missing from the cache and unreachable, so
    // the only tile it can send is the one it already sent.
    for (var i = 0; i < 5; i++) {
      await feed.tick();
    }
    expect(link.tiles.where((t) => t == here).length, 1);
  });

  test('a refused tile is retried, then given up on', () async {
    final here = deg2tile(_krakow.lat, _krakow.lon, kHeadUnitZoom);
    await seed(here);
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
    final here = deg2tile(_krakow.lat, _krakow.lon, kHeadUnitZoom);
    await seed(here);
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

  test('stopping ends the loop', () async {
    final feed = build()..setPosition(_krakow);
    feed.start();
    expect(feed.running, isTrue);
    await feed.stop();
    expect(feed.running, isFalse);
  });
}
