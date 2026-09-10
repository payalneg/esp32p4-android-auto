/// Reference values come from scripts/mapgen/tile_math.py — the same tiles the
/// generator and the Python viewer use, so the app fetches exactly what the
/// desktop tools do.
library;

import 'package:aa_bridge/nav/geo.dart';
import 'package:aa_bridge/nav/tile_math.dart';
import 'package:flutter_test/flutter_test.dart';

void main() {
  // Rynek Główny, Kraków.
  const rynek = LatLon(50.0619, 19.9368);

  test('deg2tile matches the Python generator', () {
    expect(deg2tile(rynek.lat, rynek.lon, 16), const TileId(16, 36397, 22208));
    expect(deg2tile(rynek.lat, rynek.lon, 14), const TileId(14, 9099, 5552));
  });

  test('fractional tile coordinates match too', () {
    final f = deg2tileF(rynek.lat, rynek.lon, 16);
    expect(f.x, closeTo(36397.38368, 1e-4));
    expect(f.y, closeTo(22208.649897, 1e-4));
  });

  test('tile2deg is the inverse of deg2tileF', () {
    final f = deg2tileF(rynek.lat, rynek.lon, 16);
    final back = tile2deg(f.x, f.y, 16);
    expect(back.lat, closeTo(rynek.lat, 1e-9));
    expect(back.lon, closeTo(rynek.lon, 1e-9));
  });

  test('tile2deg on whole coordinates matches Python', () {
    final p = tile2deg(36536, 22072, 16);
    expect(p.lat, closeTo(50.541363, 1e-6));
    expect(p.lon, closeTo(20.698242, 1e-6));
  });

  test('tiles are clamped to the zoom level', () {
    expect(deg2tile(89.9, 179.9, 2).x, lessThan(4));
    expect(deg2tile(-89.9, -179.9, 2).x, 0);
    expect(deg2tile(89.9, 179.9, 0), const TileId(0, 0, 0));
  });

  test('bboxTiles covers the corners in either order', () {
    final r = bboxTiles(const LatLon(50.07, 19.92), const LatLon(50.05, 19.95), 14);
    expect(r.xMin, lessThanOrEqualTo(r.xMax));
    expect(r.yMin, lessThanOrEqualTo(r.yMax));
    expect(deg2tile(50.0619, 19.9368, 14).x,
        allOf(greaterThanOrEqualTo(r.xMin), lessThanOrEqualTo(r.xMax)));
  });

  group('corridorTiles', () {
    final route = <LatLon>[
      const LatLon(50.0619, 19.9368),
      const LatLon(50.0650, 19.9500),
      const LatLon(50.0700, 19.9700),
    ];

    test('covers every tile the line passes through, plus the buffer', () {
      final tiles = corridorTiles(route, zooms: const <int>[16], maxTiles: 10000);
      for (final p in route) {
        expect(tiles, contains(deg2tile(p.lat, p.lon, 16)));
      }
      final centre = deg2tile(route.first.lat, route.first.lon, 16);
      expect(tiles, contains(TileId(16, centre.x + 1, centre.y)));
      expect(tiles, contains(TileId(16, centre.x - 1, centre.y + 1)));
    });

    test('no duplicates', () {
      final tiles = corridorTiles(route, maxTiles: 10000);
      expect(tiles.toSet().length, tiles.length);
    });

    test('a zero-buffer corridor is a thin ribbon', () {
      final thin =
          corridorTiles(route, zooms: const <int>[16], bufferTiles: 0, maxTiles: 10000);
      final fat = corridorTiles(route, zooms: const <int>[16], maxTiles: 10000);
      expect(thin.length, lessThan(fat.length));
    });

    test('the overview zoom survives truncation', () {
      final capped = corridorTiles(route, zooms: const <int>[16, 14], maxTiles: 6);
      expect(capped.length, 6);
      expect(capped.every((t) => t.z == 14), isTrue,
          reason: 'coarse tiles are cheap and go first');
    });

    test('an empty route asks for nothing', () {
      expect(corridorTiles(const <LatLon>[]), isEmpty);
      expect(corridorTiles(route, maxTiles: 0), isEmpty);
    });
  });
}
