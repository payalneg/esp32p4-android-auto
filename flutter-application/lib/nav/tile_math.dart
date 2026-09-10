/// Slippy-map tile arithmetic (Web Mercator).
/// Port of scripts/mapgen/tile_math.py.
/// https://wiki.openstreetmap.org/wiki/Slippy_map_tilenames
library;

import 'dart:math' as math;

import 'geo.dart';

/// One raster tile.
class TileId {
  const TileId(this.z, this.x, this.y);

  final int z;
  final int x;
  final int y;

  @override
  bool operator ==(Object other) =>
      other is TileId && other.z == z && other.x == x && other.y == y;

  @override
  int get hashCode => Object.hash(z, x, y);

  @override
  String toString() => '$z/$x/$y';
}

/// Degrees to fractional tile coordinates.
({double x, double y}) deg2tileF(double lat, double lon, int z) {
  final n = 1 << z;
  final x = (lon + 180.0) / 360.0 * n;
  final latRad = lat * math.pi / 180.0;
  // asinh(tan(lat)); Dart has no asinh, so spell it out.
  final t = math.tan(latRad);
  final asinh = math.log(t + math.sqrt(t * t + 1));
  final y = (1.0 - asinh / math.pi) / 2.0 * n;
  return (x: x, y: y);
}

/// Degrees to tile indices, clamped to the valid range for the zoom.
TileId deg2tile(double lat, double lon, int z) {
  final f = deg2tileF(lat, lon, z);
  final n = 1 << z;
  return TileId(
    z,
    f.x.toInt().clamp(0, n - 1),
    f.y.toInt().clamp(0, n - 1),
  );
}

/// Tile coordinates (fractional allowed) back to degrees — the tile's NW corner
/// for whole numbers.
LatLon tile2deg(double x, double y, int z) {
  final n = 1 << z;
  final lon = x / n * 360.0 - 180.0;
  final e = math.exp(math.pi * (1.0 - 2.0 * y / n));
  final lat = math.atan((e - 1 / e) / 2) * 180.0 / math.pi; // atan(sinh(...))
  return LatLon(lat, lon);
}

/// Tile index ranges covering a bounding box, inclusive on both ends.
({int xMin, int xMax, int yMin, int yMax}) bboxTiles(
    LatLon nw, LatLon se, int z) {
  final a = deg2tile(nw.lat, nw.lon, z);
  final b = deg2tile(se.lat, se.lon, z);
  return (
    xMin: math.min(a.x, b.x),
    xMax: math.max(a.x, b.x),
    yMin: math.min(a.y, b.y),
    yMax: math.max(a.y, b.y),
  );
}

/// Tiles covering a corridor around a route, ordered along it.
///
/// Ordering matters, because [maxTiles] truncates the list. Coarse zooms go
/// first — an overview layer is a handful of tiles and is what keeps the map
/// legible if the detail layer gets cut — and within a zoom the tiles follow
/// the route, so a truncated download still covers the start of the ride.
List<TileId> corridorTiles(
  List<LatLon> route, {
  List<int> zooms = const <int>[16, 14],
  int bufferTiles = 1,
  int maxTiles = 250,
}) {
  if (route.isEmpty || zooms.isEmpty || maxTiles <= 0) return const <TileId>[];
  final seen = <TileId>{};
  final out = <TileId>[];
  final cum = cumulativeM(route);
  final total = cum[cum.length - 1];

  final ordered = zooms.toList()..sort();   // overview layers first
  for (final z in ordered) {
    // Half a tile between samples so no tile the line crosses is skipped.
    final tileSpanM = 40075016.686 *
        math.cos(route.first.lat * math.pi / 180.0) /
        (1 << z);
    final step = math.max(tileSpanM / 2, 1.0);
    final samples = <LatLon>[];
    for (var d = 0.0; d < total; d += step) {
      samples.add(pointAtDistance(route, cum, d));
    }
    samples.add(route.last);
    for (final p in samples) {
      final c = deg2tile(p.lat, p.lon, z);
      final n = 1 << z;
      for (var dx = -bufferTiles; dx <= bufferTiles; dx++) {
        for (var dy = -bufferTiles; dy <= bufferTiles; dy++) {
          final x = c.x + dx;
          final y = c.y + dy;
          if (x < 0 || y < 0 || x >= n || y >= n) continue;
          final t = TileId(z, x, y);
          if (seen.add(t)) out.add(t);
        }
      }
    }
  }
  return out.length <= maxTiles ? out : out.sublist(0, maxTiles);
}
