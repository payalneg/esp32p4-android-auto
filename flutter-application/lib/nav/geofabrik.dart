/// The list of regional extracts Geofabrik publishes.
///
/// Geofabrik cuts the OSM planet into countries and provinces and serves them
/// as .osm.pbf, which is the same input scripts/mapgen uses. Their index is a
/// GeoJSON-shaped catalogue of every region and its download URL — so the app
/// can offer the real list rather than a hard-coded one.
library;

import 'dart:convert';
import 'dart:io';

const String kGeofabrikIndexUrl =
    'https://download.geofabrik.de/index-v1-nogeom.json';

/// One downloadable extract.
class GeofabrikRegion {
  const GeofabrikRegion({
    required this.id,
    required this.name,
    required this.pbfUrl,
    this.parent,
  });

  /// Path-like identifier, e.g. `poland/malopolskie`.
  final String id;

  /// Human name, e.g. `Małopolskie`.
  final String name;
  final Uri pbfUrl;

  /// Id of the region this one sits inside, if any.
  final String? parent;

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
    // Deeper ids are more specific, and specificity is what a rider wants.
    hits.sort((a, b) {
      final depth = '/'.allMatches(b.id).length - '/'.allMatches(a.id).length;
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
      out.add(GeofabrikRegion(
        id: id,
        name: cleanName(name),
        pbfUrl: Uri.parse(pbf),
        parent: parent is String ? parent : null,
      ));
    }
    if (out.isEmpty) throw GeofabrikException('mapdata.err.catalogFormat');
    return GeofabrikIndex(out);
  }

  static Future<GeofabrikIndex> fetch({String? userAgent}) async {
    final client = HttpClient();
    if (userAgent != null) client.userAgent = userAgent;
    try {
      final resp = await (await client.getUrl(Uri.parse(kGeofabrikIndexUrl)))
          .close()
          .timeout(const Duration(seconds: 30));
      if (resp.statusCode != 200) {
        await resp.drain<void>();
        throw GeofabrikException(
            'mapdata.err.http', <String, String>{'code': '${resp.statusCode}'});
      }
      return parse(await resp.transform(utf8.decoder).join());
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
