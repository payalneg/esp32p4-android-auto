/// The list of regional extracts Geofabrik publishes.
///
/// Geofabrik cuts the OSM planet into countries and provinces and serves them
/// as .osm.pbf, which is the same input scripts/mapgen uses. Their index is a
/// GeoJSON-shaped catalogue of every region and its download URL — so the app
/// can offer the real list rather than a hard-coded one.
library;

import 'dart:convert';
import 'dart:io';

import 'geo.dart';

const String kGeofabrikIndexUrl =
    'https://download.geofabrik.de/index-v1-nogeom.json';

/// The same catalogue with each region's outline. Three and a half megabytes
/// against half of one, so it is only fetched when a position has to be turned
/// into a region (see [GeofabrikIndex.regionAt]).
const String kGeofabrikGeometryIndexUrl =
    'https://download.geofabrik.de/index-v1.json';

/// One downloadable extract.
class GeofabrikRegion {
  const GeofabrikRegion({
    required this.id,
    required this.name,
    required this.pbfUrl,
    this.parent,
    this.shape,
  });

  /// Path-like identifier, e.g. `malopolskie` or `us/alabama`.
  final String id;

  /// Human name, e.g. `Małopolskie`.
  final String name;
  final Uri pbfUrl;

  /// Id of the region this one sits inside, if any.
  final String? parent;

  /// The region's outline, when the catalogue was fetched with geometry.
  final RegionShape? shape;

  /// `Poland / Małopolskie` — what the list shows.
  String pathLabel(Map<String, GeofabrikRegion> byId) {
    final parts = <String>[name];
    var at = parent;
    var guard = 0;
    while (at != null && guard++ < 8) {
      final up = byId[at];
      if (up == null) break;
      parts.insert(0, up.name);
      at = up.parent;
    }
    return parts.join(' / ');
  }

  /// `Poland / Małopolskie` — the parent and the region, without the continent
  /// on top: what fits on one line of a banner on a phone.
  String localLabel(Map<String, GeofabrikRegion> byId) {
    final up = parent == null ? null : byId[parent]?.name;
    return up == null ? name : '$up / $name';
  }
}

/// A region boundary.
///
/// GeoJSON's nesting is kept (polygons → rings → [lon, lat] pairs) but each
/// ring is flattened to a single `[lon, lat, lon, lat, …]` list: the point
/// test touches it for every fix, and an object per coordinate buys nothing.
class RegionShape {
  RegionShape._(this.polygons, this.bounds);

  final List<List<List<double>>> polygons;
  final LatLonBounds bounds;

  /// [geometry] is a GeoJSON geometry object (Polygon or MultiPolygon), with
  /// a ring's first point repeated at the end, as GeoJSON requires.
  static RegionShape? fromGeoJson(Object? geometry) {
    if (geometry is! Map) return null;
    final type = geometry['type'];
    if (type != 'Polygon' && type != 'MultiPolygon') return null;
    final raw = geometry['coordinates'];
    if (raw is! List) return null;
    final polygons = <List<List<double>>>[];
    if (type == 'Polygon') {
      final p = _polygon(raw);
      if (p != null) polygons.add(p);
    } else {
      for (final poly in raw) {
        if (poly is! List) continue;
        final p = _polygon(poly);
        if (p != null) polygons.add(p);
      }
    }
    if (polygons.isEmpty) return null;
    return RegionShape._(polygons, _boundsOf(polygons));
  }

  bool contains(LatLon p) {
    if (!bounds.contains(p)) return false;
    // GeoJSON: the first ring is the outline, the rest are holes.
    for (final poly in polygons) {
      if (poly.isEmpty || !_ringContains(poly.first, p)) continue;
      var inHole = false;
      for (var r = 1; r < poly.length; r++) {
        if (_ringContains(poly[r], p)) {
          inHole = true;
          break;
        }
      }
      if (!inHole) return true;
    }
    return false;
  }

  static List<List<double>>? _polygon(Object? raw) {
    if (raw is! List) return null;
    final rings = <List<double>>[];
    for (final ring in raw) {
      final flat = _ring(ring);
      if (flat != null) rings.add(flat);
    }
    return rings.isEmpty ? null : rings;
  }

  static List<double>? _ring(Object? raw) {
    if (raw is! List) return null;
    final out = <double>[];
    for (final point in raw) {
      if (point is! List || point.length < 2) continue;
      final lon = point[0];
      final lat = point[1];
      if (lon is! num || lat is! num) continue;
      out
        ..add(lon.toDouble())
        ..add(lat.toDouble());
    }
    return out.length >= 6 ? out : null; // at least a triangle
  }

  static bool _ringContains(List<double> ring, LatLon p) {
    var inside = false;
    final x = p.lon;
    final y = p.lat;
    final n = ring.length;
    for (var i = 0, j = n - 2; i < n; j = i, i += 2) {
      final xi = ring[i], yi = ring[i + 1];
      final xj = ring[j], yj = ring[j + 1];
      if ((yi > y) != (yj > y) &&
          x < (xj - xi) * (y - yi) / (yj - yi) + xi) {
        inside = !inside;
      }
    }
    return inside;
  }

  static LatLonBounds _boundsOf(List<List<List<double>>> polygons) {
    var south = 90.0, west = 180.0, north = -90.0, east = -180.0;
    for (final poly in polygons) {
      for (final ring in poly) {
        for (var i = 0; i < ring.length; i += 2) {
          final lon = ring[i], lat = ring[i + 1];
          if (lat < south) south = lat;
          if (lat > north) north = lat;
          if (lon < west) west = lon;
          if (lon > east) east = lon;
        }
      }
    }
    return LatLonBounds(south: south, west: west, north: north, east: east);
  }
}

/// Strips the markup Geofabrik puts in its names.
///
/// Several entries carry a literal `<br />` between the local name and the
/// English one — the index is generated from their web pages — and a raw tag
/// in a list of places to download looks like a bug, because it is one.
String cleanName(String raw) => raw
    .replaceAll(RegExp(r'<br\s*/?>', caseSensitive: false), ' ')
    .replaceAll(RegExp(r'<[^>]*>'), '')
    .replaceAll(RegExp(r'\s+'), ' ')
    .trim();

class GeofabrikException implements Exception {
  GeofabrikException(this.messageKey, [this.args]);
  final String messageKey;
  final Map<String, String>? args;
}

class GeofabrikIndex {
  GeofabrikIndex(this.regions) : byId = <String, GeofabrikRegion>{
          for (final r in regions) r.id: r,
        };

  final List<GeofabrikRegion> regions;
  final Map<String, GeofabrikRegion> byId;

  /// How many parents a region has: a province is deeper than its country,
  /// which is deeper than its continent. Used to prefer the download that is
  /// actually local when two outlines contain the same point.
  int depthOf(GeofabrikRegion r) {
    var depth = 0;
    var at = r.parent;
    while (at != null && depth < 16) {
      depth++;
      at = byId[at]?.parent;
    }
    return depth;
  }

  /// The smallest region containing [p], or null when the catalogue covers
  /// none — open sea, or a position the outlines are too coarse for.
  ///
  /// Deepest wins (a province over its country), and for two regions at the
  /// same depth the tighter outline wins: where the catalogue overlaps, the
  /// more specific file is the better answer.
  GeofabrikRegion? regionAt(LatLon p) {
    GeofabrikRegion? best;
    var bestDepth = -1;
    var bestArea = double.infinity;
    for (final r in regions) {
      final shape = r.shape;
      if (shape == null || !shape.contains(p)) continue;
      final depth = depthOf(r);
      final area = shape.bounds.areaDeg2;
      if (depth > bestDepth || (depth == bestDepth && area < bestArea)) {
        best = r;
        bestDepth = depth;
        bestArea = area;
      }
    }
    return best;
  }

  /// Regions whose name or path matches [query], smallest first — a province
  /// is a better answer than the continent containing it.
  List<GeofabrikRegion> search(String query, {int limit = 30}) {
    final q = query.trim().toLowerCase();
    if (q.isEmpty) return const <GeofabrikRegion>[];
    final hits = <GeofabrikRegion>[];
    for (final r in regions) {
      if (r.name.toLowerCase().contains(q) || r.id.toLowerCase().contains(q)) {
        hits.add(r);
      }
    }
    // Deeper regions are more specific, and specificity is what a rider wants.
    hits.sort((a, b) {
      final depth = depthOf(b) - depthOf(a);
      return depth != 0 ? depth : a.name.compareTo(b.name);
    });
    return hits.length <= limit ? hits : hits.sublist(0, limit);
  }

  static GeofabrikIndex parse(String body) {
    final Object? decoded;
    try {
      decoded = jsonDecode(body);
    } on FormatException {
      throw GeofabrikException('mapdata.err.catalogFormat');
    }
    if (decoded is! Map<String, Object?>) {
      throw GeofabrikException('mapdata.err.catalogFormat');
    }
    final features = decoded['features'];
    if (features is! List) {
      throw GeofabrikException('mapdata.err.catalogFormat');
    }
    final out = <GeofabrikRegion>[];
    for (final feature in features) {
      if (feature is! Map<String, Object?>) continue;
      final props = feature['properties'];
      if (props is! Map<String, Object?>) continue;
      final id = props['id'];
      final name = props['name'];
      final urls = props['urls'];
      if (id is! String || name is! String || urls is! Map) continue;
      final pbf = urls['pbf'];
      if (pbf is! String) continue;
      final parent = props['parent'];
      final shape = RegionShape.fromGeoJson(feature['geometry']);
      out.add(GeofabrikRegion(
        id: id,
        name: cleanName(name),
        pbfUrl: Uri.parse(pbf),
        parent: parent is String ? parent : null,
        shape: shape,
      ));
    }
    if (out.isEmpty) throw GeofabrikException('mapdata.err.catalogFormat');
    return GeofabrikIndex(out);
  }

  /// Fetches the catalogue, keeping a copy at [cache].
  ///
  /// The index changes daily at most, and the region picker used to re-download
  /// it every time it opened. A cached copy younger than [maxAge] is used as
  /// is; an older one is refreshed but still stands in when the network is
  /// gone, because an offline rider asking "which region am I in" has no other
  /// way to get an answer.
  ///
  /// [geometry] selects the three-and-a-half-megabyte variant with region
  /// outlines, which is what [regionAt] needs.
  static Future<GeofabrikIndex> fetch({
    String? userAgent,
    bool geometry = false,
    File? cache,
    bool refresh = false,
    Duration maxAge = const Duration(days: 7),
  }) async {
    String? stale;
    if (cache != null && await cache.exists()) {
      try {
        stale = utf8.decode(await cache.readAsBytes());
        if (!refresh) {
          final age = DateTime.now().difference((await cache.stat()).modified);
          if (age < maxAge) {
            try {
              return parse(stale);
            } on GeofabrikException {
              stale = null; // a truncated cache is no cache
            }
          }
        }
      } on Object {
        stale = null;
      }
    }

    final String body;
    try {
      body = await _download(
          geometry ? kGeofabrikGeometryIndexUrl : kGeofabrikIndexUrl,
          userAgent: userAgent);
    } on GeofabrikException {
      if (stale != null) {
        try {
          return parse(stale);
        } on GeofabrikException {
          // fall through and report the download failure
        }
      }
      rethrow;
    }

    if (cache != null) {
      try {
        await cache.parent.create(recursive: true);
        final part = File('${cache.path}.part');
        await part.writeAsString(body, flush: true);
        await part.rename(cache.path);
      } on FileSystemException {
        // The cache is an optimisation; failing to keep it is not a failure.
      }
    }
    return parse(body);
  }

  static Future<String> _download(String url, {String? userAgent}) async {
    final client = HttpClient();
    if (userAgent != null) client.userAgent = userAgent;
    try {
      final resp = await (await client.getUrl(Uri.parse(url)))
          .close()
          .timeout(const Duration(seconds: 30));
      if (resp.statusCode != 200) {
        await resp.drain<void>();
        throw GeofabrikException(
            'mapdata.err.http', <String, String>{'code': '${resp.statusCode}'});
      }
      return await resp
          .transform(utf8.decoder)
          .join()
          .timeout(const Duration(seconds: 90));
    } on GeofabrikException {
      rethrow;
    } on Object catch (e) {
      throw GeofabrikException(
          'mapdata.err.download', <String, String>{'err': '$e'});
    } finally {
      client.close(force: true);
    }
  }
}
