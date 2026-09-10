/// The two things the provider must get right about flutter_map: which real
/// tile a layer coordinate stands for, and which request batch it came from.
library;

import 'dart:io';

import 'package:aa_bridge/nav/tile_cache.dart';
import 'package:aa_bridge/ui/nav/cached_tile_provider.dart';
import 'package:flutter_map/flutter_map.dart';
import 'package:flutter_test/flutter_test.dart';

void main() {
  late Directory root;
  late CachedTileProvider provider;

  setUp(() {
    root = Directory.systemTemp.createTempSync('tileprov');
    provider = CachedTileProvider(TileCache(root, userAgent: 'test/1.0'));
  });

  tearDown(() {
    if (root.existsSync()) root.deleteSync(recursive: true);
  });

  TileLayer layer({bool retina = false}) => TileLayer(
        urlTemplate: 'https://example.invalid/{z}/{x}/{y}.png',
        tileProvider: provider,
        retinaMode: retina,
        maxZoom: 20,
        maxNativeZoom: 19,
      );

  group('tileIdFor', () {
    test('plain layer: the coordinate is the tile', () {
      final t = CachedTileProvider.tileIdFor(
          const TileCoordinates(36609, 22069, 16), layer());
      expect('$t', '16/36609/22069');
    });

    test('retina simulation: one level deeper than the layer numbers it', () {
      // The bug this pins: zoom-20 x/y sent with z=19 came back HTTP 400 for
      // every tile, and the map was black.
      final l = layer(retina: true);
      expect(l.zoomOffset, 1.0);
      final t = CachedTileProvider.tileIdFor(
          const TileCoordinates(582465, 355404, 19), l);
      expect('$t', '20/582465/355404');
      expect(t.x, lessThan(1 << t.z), reason: 'a valid tile at that zoom');
    });

    test('retina layer never asks deeper than OSM serves', () {
      final l = layer(retina: true);
      // The layer clamps its own zoom to maxNativeZoom - 1 = 18 and adds the
      // offset back, so the deepest tile actually requested is 19.
      expect(l.maxNativeZoom, 18);
      final t = CachedTileProvider.tileIdFor(
          TileCoordinates(0, 0, l.maxNativeZoom), l);
      expect(t.z, 19);
    });
  });

  group('request generations', () {
    test('one synchronous batch shares a number; the next pass gets a new one',
        () async {
      final l = layer();
      provider.getImage(const TileCoordinates(1, 1, 16), l);
      provider.getImage(const TileCoordinates(1, 2, 16), l);
      provider.getImage(const TileCoordinates(2, 1, 16), l);
      expect(provider.generation, 1, reason: 'centre-out, same viewport');

      await Future<void>.value(); // the batch yields — a new camera event
      provider.getImage(const TileCoordinates(9, 9, 16), l);
      expect(provider.generation, 2);
      provider.getImage(const TileCoordinates(9, 8, 16), l);
      expect(provider.generation, 2);
    });
  });
}
