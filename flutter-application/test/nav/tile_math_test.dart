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

  group('tileUrls', () {
    test('offers the OSMF server first, then the mirrors', () {
      final urls = tileUrls(const TileId(16, 36397, 22208));
      expect(urls, hasLength(kTileMirrors.length));
      expect(urls.first.host, 'tile.openstreetmap.org');
      expect(urls.first.path, '/16/36397/22208.png');
      expect(urls.map((u) => u.host).toSet().length, urls.length,
          reason: 'a mirror that is the same host is not a mirror');
      for (final u in urls) {
        expect(u.scheme, 'https');
        expect(u.path, endsWith('/16/36397/22208.png'));
      }
    });
  });

  group('areaPyramid', () {
    const rynek = LatLon(50.0619, 19.9368);

    test('covers every zoom in the range, coarse first', () {
      final tiles =
          areaPyramid(rynek, radiusM: 500, zoomMin: 14, zoomMax: 17,
              maxTiles: 100000);
      expect(tiles.map((t) => t.z).toSet(), <int>{14, 15, 16, 17});
      expect(tiles.first.z, 14, reason: 'the overview goes down first');
      expect(tiles.last.z, 17);
    });

    test('never asks for more than the budget', () {
      final tiles = areaPyramid(rynek,
          radiusM: 500, zoomMin: 14, zoomMax: 19, maxTiles: 20);
      expect(tiles.length, lessThanOrEqualTo(20));
      expect(tiles.first.z, 14, reason: 'the overview goes down first');
    });

    test('a wide area still covers its coarse levels in full', () {
      // A hundred kilometres: the point of the graduation is that the level
      // you zoom out to is complete, even though zoom 19 cannot be.
      final tiles = areaPyramid(rynek,
          radiusM: 100000, zoomMin: 12, zoomMax: 19, maxTiles: 3000);
      final byZoom = <int, int>{};
      for (final t in tiles) {
        byZoom[t.z] = (byZoom[t.z] ?? 0) + 1;
      }
      final full = tilesAround(rynek,
          radiusM: 100000, zooms: const <int>[12], maxTiles: 100000).length;
      expect(byZoom[12], full, reason: 'the overview is not truncated');
      expect(byZoom[19], greaterThan(0), reason: 'and detail still shows up');
    });

    test('a small area comes down complete at every level', () {
      final tiles = areaPyramid(rynek,
          radiusM: 800, zoomMin: 12, zoomMax: 19, maxTiles: 100000);
      for (var z = 12; z <= 19; z++) {
        final full = tilesAround(rynek,
            radiusM: 800, zooms: <int>[z], maxTiles: 100000).length;
        expect(tiles.where((t) => t.z == z).length, full, reason: 'z$z');
      }
    });

    test('asking for nothing returns nothing', () {
      expect(areaPyramid(rynek, radiusM: 500, maxTiles: 0), isEmpty);
    });
  });

  group('tilesAround', () {
    const rynek = LatLon(50.0619, 19.9368);

    test('covers a disc and includes the tile under the point', () {
      final tiles = tilesAround(rynek, radiusM: 300, zooms: const <int>[19]);
      expect(tiles, contains(deg2tile(rynek.lat, rynek.lon, 19)));
      expect(tiles.length, greaterThan(9));
    });

    test('nearest first, so a cap keeps the ground underfoot', () {
      final centre = deg2tile(rynek.lat, rynek.lon, 19);
      final tiles =
          tilesAround(rynek, radiusM: 600, zooms: const <int>[19], maxTiles: 9);
      expect(tiles.length, 9);
      expect(tiles.first, centre);
      for (final t in tiles) {
        expect((t.x - centre.x).abs(), lessThanOrEqualTo(1));
        expect((t.y - centre.y).abs(), lessThanOrEqualTo(1));
      }
    });

    test('a bigger radius asks for more', () {
      // Past the default cap both would come back the same length, which is
      // the cap doing its job rather than the radius being ignored.
      final near = tilesAround(rynek,
          radiusM: 200, zooms: const <int>[19], maxTiles: 10000);
      final far = tilesAround(rynek,
          radiusM: 800, zooms: const <int>[19], maxTiles: 10000);
      expect(far.length, greaterThan(near.length));
    });

    test('the cap is what limits a wide radius', () {
      expect(
          tilesAround(rynek, radiusM: 5000, zooms: const <int>[19]).length, 60);
    });

    test('several zooms come back together, without duplicates', () {
      final tiles =
          tilesAround(rynek, radiusM: 400, zooms: const <int>[19, 17]);
      expect(tiles.map((t) => t.z).toSet(), <int>{17, 19});
      expect(tiles.toSet().length, tiles.length);
    });

    test('asking for nothing returns nothing', () {
      expect(tilesAround(rynek, maxTiles: 0), isEmpty);
    });
  });

  group('tilesInBounds', () {
    test('covers the box and starts from the middle', () {
      const nw = LatLon(50.07, 19.92);
      const se = LatLon(50.05, 19.95);
      final tiles = tilesInBounds(nw, se, 16);
      final r = bboxTiles(nw, se, 16);
      expect(tiles.length, (r.xMax - r.xMin + 1) * (r.yMax - r.yMin + 1));
      final first = tiles.first;
      expect(first.x, closeTo((r.xMin + r.xMax) / 2, 1));
      expect(first.y, closeTo((r.yMin + r.yMax) / 2, 1));
    });

    test('the cap cuts the outside, not the middle', () {
      const nw = LatLon(50.07, 19.92);
      const se = LatLon(50.05, 19.95);
      final full = tilesInBounds(nw, se, 16);
      final capped = tilesInBounds(nw, se, 16, maxTiles: 4);
      expect(capped.length, 4);
      expect(capped, full.take(4));
    });
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
