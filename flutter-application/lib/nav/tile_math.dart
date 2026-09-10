/// Slippy-map tile arithmetic (Web Mercator).
/// Port of scripts/mapgen/tile_math.py.
/// https://wiki.openstreetmap.org/wiki/Slippy_map_tilenames
library;

import 'dart:math' as math;

import 'geo.dart';

/// Tile servers that answer without an API key, in the order they are tried.
///
/// The OSMF server comes first; the others are mirrors of the same map, used
/// only when it refuses or fails, so a blocked moment does not leave a blank
/// screen. All are donated capacity and all require the OpenStreetMap
/// attribution the map already carries.
/// See https://wiki.openstreetmap.org/wiki/Raster_tile_providers
const List<String> kTileMirrors = <String>[
  'https://tile.openstreetmap.org/{z}/{x}/{y}.png',
  'https://tile.openstreetmap.de/{z}/{x}/{y}.png',
  'https://a.tile.openstreetmap.fr/osmfr/{z}/{x}/{y}.png',
];

/// The addresses to try for one tile, primary first.
List<Uri> tileUrls(TileId t) => <Uri>[
      for (final template in kTileMirrors)
        Uri.parse(template
            .replaceAll('{z}', '${t.z}')
            .replaceAll('{x}', '${t.x}')
            .replaceAll('{y}', '${t.y}')),
    ];

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

/// Tiles covering a disc of [radiusM] around [centre], nearest first.
///
/// Nearest first matters: [maxTiles] truncates the list, and the ground under
/// the rider is worth more than the ground behind the horizon. Used to keep
/// the map ahead cached while riding.
List<TileId> tilesAround(
  LatLon centre, {
  double radiusM = 600,
  List<int> zooms = const <int>[16],
  int maxTiles = 60,
}) {
  if (maxTiles <= 0) return const <TileId>[];
  final out = <TileId>[];
  final seen = <TileId>{};
  for (final z in zooms.toList()..sort()) {
    final n = 1 << z;
    // Tile side in metres at this latitude, so the radius becomes a tile count.
    final tileSpanM =
        40075016.686 * math.cos(centre.lat * math.pi / 180.0) / n;
    final ring = (radiusM / tileSpanM).ceil();
    final middle = deg2tile(centre.lat, centre.lon, z);
    final candidates = <({TileId tile, int distance})>[];
    for (var dx = -ring; dx <= ring; dx++) {
      for (var dy = -ring; dy <= ring; dy++) {
        final x = middle.x + dx;
        final y = middle.y + dy;
        if (x < 0 || y < 0 || x >= n || y >= n) continue;
        // Chebyshev distance keeps it a square of rings, which is what the
        // viewport actually is.
        candidates.add((
          tile: TileId(z, x, y),
          distance: math.max(dx.abs(), dy.abs()),
        ));
      }
    }
    candidates.sort((a, b) => a.distance.compareTo(b.distance));
    for (final c in candidates) {
      if (seen.add(c.tile)) out.add(c.tile);
    }
  }
  return out.length <= maxTiles ? out : out.sublist(0, maxTiles);
}

/// A whole area at every scale, coarse levels first.
///
/// Offline means being able to zoom out to see where you are heading and in
/// to read a house number, so a saved area is a pyramid rather than one
/// level. Coarse zooms go first: they are nearly free and they are what keeps
/// the map legible if the tile budget cuts the download short. Within a level
/// the tiles run outwards from the middle.
List<TileId> areaPyramid(
  LatLon centre, {
  required double radiusM,
  int zoomMin = 13,
  int zoomMax = 19,
  int maxTiles = 3000,
}) {
  if (maxTiles <= 0) return const <TileId>[];
  final out = <TileId>[];
  for (var z = zoomMin; z <= zoomMax && out.length < maxTiles; z++) {
    out.addAll(tilesAround(centre,
        radiusM: radiusM, zooms: <int>[z], maxTiles: maxTiles - out.length));
  }
  return out;
}

/// How many tiles a full [areaPyramid] would be, without building the list —
/// so the user can be told the size before agreeing to it.
int areaPyramidCount(LatLon centre,
    {required double radiusM, int zoomMin = 13, int zoomMax = 19}) {
  var total = 0;
  for (var z = zoomMin; z <= zoomMax; z++) {
    final span =
        40075016.686 * math.cos(centre.lat * math.pi / 180.0) / (1 << z);
    final ring = (radiusM / span).ceil();
    total += (2 * ring + 1) * (2 * ring + 1);
  }
  return total;
}

/// Every tile of [z] inside [bounds], ordered outwards from the middle.
///
/// Centre-first because a cap will cut the tail, and the middle of the area a
/// rider chose is the part they meant.
List<TileId> tilesInBounds(LatLon nw, LatLon se, int z, {int maxTiles = 2000}) {
  if (maxTiles <= 0) return const <TileId>[];
  final r = bboxTiles(nw, se, z);
  final cx = (r.xMin + r.xMax) / 2;
  final cy = (r.yMin + r.yMax) / 2;
  final all = <({TileId tile, double d})>[];
  for (var x = r.xMin; x <= r.xMax; x++) {
    for (var y = r.yMin; y <= r.yMax; y++) {
      final dx = x - cx;
      final dy = y - cy;
      all.add((tile: TileId(z, x, y), d: dx * dx + dy * dy));
    }
  }
  all.sort((a, b) => a.d.compareTo(b.d));
  final out = <TileId>[for (final e in all) e.tile];
  return out.length <= maxTiles ? out : out.sublist(0, maxTiles);
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
