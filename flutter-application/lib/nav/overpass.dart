/// Fetches raw OSM data for an area straight from an Overpass server.
///
/// Overpass is the OSM project's own query API: ask for the roads inside a
/// bounding box and it answers with the ways and the nodes they reference —
/// exactly the two things graph_builder.dart needs. Compared with downloading
/// a regional `.osm.pbf` extract this is a few megabytes rather than a few
/// hundred, and it needs no PBF decoder on the phone.
///
/// Two constraints shape this. Overpass instances are donated capacity, so the
/// client names itself, asks for bounded areas, and gives up rather than
/// retrying a refusal. And a city centre answers with roughly a megabyte of
/// JSON per square kilometre, which balloons several times over once decoded —
/// so a request is split into cells, each decoded and reduced to compact
/// structures before the next one is asked for. Peak memory is then one cell,
/// whatever the size of the area.
library;

import 'dart:async';
import 'dart:convert';
import 'dart:io';
import 'dart:math' as math;

import 'geo.dart';
import 'graph_builder.dart';
import 'osm_ways.dart';

/// Public Overpass instances that take queries without a key.
///
/// The FOSSGIS one is canonical but describes itself as overloaded, so a
/// multi-cell download rotates across all of them: it finishes sooner and no
/// single volunteer server carries the whole job. Order is the fallback order
/// for any one cell.
/// See https://wiki.openstreetmap.org/wiki/Overpass_API
const List<String> kOverpassEndpoints = <String>[
  'https://overpass-api.de/api/interpreter',
  'https://overpass.kumi.systems/api/interpreter',
  'https://overpass.private.coffee/api/interpreter',
  'https://maps.mail.ru/osm/tools/overpass/api/interpreter',
];

/// Above this the request is refused before anything is sent. At roughly a
/// megabyte of JSON per square kilometre downtown, 150 km² is already a long
/// download; more than that is not a ride, it is a region, and belongs in
/// scripts/mapgen.
const double kMaxAreaKm2 = 150;

/// Each request covers at most this much ground. Small enough that one
/// decoded response stays a manageable object graph on a phone.
const double kCellKm2 = 9;

/// Reported as an i18n key so the UI can localise it.
class OverpassException implements Exception {
  OverpassException(this.messageKey, [this.args]);
  final String messageKey;
  final Map<String, String>? args;
}

/// A rectangle of the world, in degrees.
class GeoBounds {
  const GeoBounds({
    required this.south,
    required this.west,
    required this.north,
    required this.east,
  });

  final double south;
  final double west;
  final double north;
  final double east;

  /// Rough area in square kilometres — good enough to refuse a silly request.
  double get areaKm2 {
    const kmPerDegree = 111.32;
    final midLat = (south + north) / 2 * math.pi / 180;
    final h = (north - south) * kmPerDegree;
    final w = (east - west) * kmPerDegree * math.cos(midLat);
    return (h * w).abs();
  }
  /// Splits into cells of at most [maxCellKm2], so each request decodes on its
  /// own. A single cell comes back as itself.
  List<GeoBounds> split({double maxCellKm2 = kCellKm2}) {
    final area = areaKm2;
    if (area <= maxCellKm2) return <GeoBounds>[this];
    // Square-ish cells: divide each side by the same factor.
    final factor = math.sqrt(area / maxCellKm2).ceil();
    final dLat = (north - south) / factor;
    final dLon = (east - west) / factor;
    final out = <GeoBounds>[];
    for (var i = 0; i < factor; i++) {
      for (var j = 0; j < factor; j++) {
        out.add(GeoBounds(
          south: south + dLat * i,
          west: west + dLon * j,
          north: i == factor - 1 ? north : south + dLat * (i + 1),
          east: j == factor - 1 ? east : west + dLon * (j + 1),
        ));
      }
    }
    return out;
  }
}

class OverpassResult {
  const OverpassResult({
    required this.ways,
    required this.nodes,
    required this.places,
    required this.bytes,
  });

  final List<OsmWay> ways;
  final Map<int, LatLon> nodes;
  final List<OsmPlace> places;

  /// Size of the response, for the "downloaded N MB" line.
  final int bytes;
}

class OverpassClient {
  OverpassClient({
    HttpClient Function()? clientFactory,
    List<String>? endpoints,
  })  : _clientFactory = clientFactory ?? (() => HttpClient()),
        _endpoints = endpoints ?? kOverpassEndpoints;

  final HttpClient Function() _clientFactory;
  final List<String> _endpoints;

  /// Everything the router needs for [bounds]: the road network, and named
  /// places for search. One query, so the server plans it once.
  static String buildQuery(GeoBounds b, {int timeoutS = 180}) {
    final bbox = '${b.south},${b.west},${b.north},${b.east}';
    return '[out:json][timeout:$timeoutS];'
        '('
        'way["highway"]($bbox);'
        'node["amenity"]($bbox);'
        'node["shop"]($bbox);'
        'node["tourism"]($bbox);'
        ');'
        'out body;'
        '>;'
        'out skel qt;';
  }

  /// Everything the router needs for [bounds], cell by cell.
  ///
  /// Ways are deduplicated by id: Overpass returns a whole way whenever any of
  /// it falls inside a cell, so a street crossing a cell boundary comes back
  /// twice and would otherwise be built into the graph twice.
  Future<OverpassResult> fetch(
    GeoBounds bounds, {
    String? userAgent,
    void Function(int cell, int cells, int bytes)? onProgress,
  }) async {
    if (bounds.areaKm2 > kMaxAreaKm2) {
      throw OverpassException('mapdata.err.areaTooBig', <String, String>{
        'km2': bounds.areaKm2.round().toString(),
        'max': kMaxAreaKm2.round().toString(),
      });
    }
    final cells = bounds.split();
    final nodes = <int, LatLon>{};
    final ways = <OsmWay>[];
    final places = <OsmPlace>[];
    final seenWays = <int>{};
    var totalBytes = 0;

    for (var i = 0; i < cells.length; i++) {
      // Start each cell at a different server, so a long download is spread
      // rather than aimed at one of them.
      final body = await _request(
        cells[i],
        userAgent,
        startAt: i,
        // Report while the cell is still arriving: a single update per cell
        // leaves the figure frozen for the minute one takes, which reads
        // exactly like a hang.
        onBytes: (soFar) => onProgress?.call(i + 1, cells.length,
            totalBytes + soFar),
      );
      totalBytes += body.length;
      final part = parse(body, body.length, seenWays: seenWays);
      nodes.addAll(part.nodes);
      ways.addAll(part.ways);
      places.addAll(part.places);
      onProgress?.call(i + 1, cells.length, totalBytes);
      // Be a good guest between cells.
      if (i + 1 < cells.length) {
        await Future<void>.delayed(const Duration(milliseconds: 500));
      }
    }
    return OverpassResult(
        ways: ways, nodes: nodes, places: places, bytes: totalBytes);
  }

  /// One cell, from the first endpoint that will serve it, beginning at
  /// [startAt] so consecutive cells do not all land on the same server.
  Future<String> _request(GeoBounds cell, String? userAgent,
      {int startAt = 0, void Function(int bytesSoFar)? onBytes}) async {
    Object? lastError;
    for (var hop = 0; hop < _endpoints.length; hop++) {
      final endpoint = _endpoints[(startAt + hop) % _endpoints.length];
      final client = _clientFactory();
      if (userAgent != null) client.userAgent = userAgent;
      try {
        final req = await client.postUrl(Uri.parse(endpoint));
        req.headers.contentType = ContentType(
            'application', 'x-www-form-urlencoded',
            charset: 'utf-8');
        req.write('data=${Uri.encodeQueryComponent(buildQuery(cell))}');
        final resp = await req.close().timeout(const Duration(minutes: 4));
        if (resp.statusCode == 429 || resp.statusCode == 504) {
          await resp.drain<void>();
          throw OverpassException('mapdata.err.overpassBusy');
        }
        if (resp.statusCode != 200) {
          await resp.drain<void>();
          throw OverpassException('mapdata.err.http',
              <String, String>{'code': '${resp.statusCode}'});
        }
        final buffer = StringBuffer();
        var received = 0;
        await for (final chunk in resp.transform(utf8.decoder)) {
          buffer.write(chunk);
          received += chunk.length;
          onBytes?.call(received);
        }
        return buffer.toString();
      } on OverpassException catch (e) {
        // A busy server is worth trying the next mirror for; a refusal is not.
        if (e.messageKey != 'mapdata.err.overpassBusy') rethrow;
        lastError = e;
      } on Object catch (e) {
        lastError = e;
      } finally {
        client.close(force: true);
      }
    }
    if (lastError is OverpassException) throw lastError;
    throw OverpassException(
        'mapdata.err.download', <String, String>{'err': '$lastError'});
  }

  /// Splits an Overpass JSON answer into what the builder consumes.
  static OverpassResult parse(String body, int bytes,
      {Set<int>? seenWays}) {
    final Object? decoded;
    try {
      decoded = jsonDecode(body);
    } on FormatException {
      throw OverpassException('mapdata.err.overpassFormat');
    }
    if (decoded is! Map<String, Object?>) {
      throw OverpassException('mapdata.err.overpassFormat');
    }
    final elements = decoded['elements'];
    if (elements is! List) {
      throw OverpassException('mapdata.err.overpassFormat');
    }

    final nodes = <int, LatLon>{};
    final ways = <OsmWay>[];
    final places = <OsmPlace>[];

    for (final element in elements) {
      if (element is! Map<String, Object?>) continue;
      switch (element['type']) {
        case 'node':
          final id = element['id'];
          final lat = element['lat'];
          final lon = element['lon'];
          if (id is! num || lat is! num || lon is! num) continue;
          final at = LatLon(lat.toDouble(), lon.toDouble());
          nodes[id.toInt()] = at;
          final tags = _tags(element['tags']);
          if (tags != null) {
            final entry = searchEntry(tags);
            if (entry != null) {
              places.add(OsmPlace(entry.display, entry.kind, at));
            }
          }
        case 'way':
          final wayId = element['id'];
          if (wayId is num && seenWays != null && !seenWays.add(wayId.toInt())) {
            continue; // already taken from a neighbouring cell
          }
          final tags = _tags(element['tags']);
          if (tags == null) continue;
          final wayClass = classifyWay(tags);
          if (wayClass == null) continue;
          final refs = element['nodes'];
          if (refs is! List || refs.length < 2) continue;
          ways.add(OsmWay(
            wayClass: wayClass,
            direction: wayDirection(tags),
            nodeIds: <int>[
              for (final r in refs)
                if (r is num) r.toInt(),
            ],
          ));
      }
    }
    return OverpassResult(
        ways: ways, nodes: nodes, places: places, bytes: bytes);
  }

  static Map<String, String>? _tags(Object? raw) {
    if (raw is! Map) return null;
    final out = <String, String>{};
    raw.forEach((k, v) {
      if (k is String && v is String) out[k] = v;
    });
    return out;
  }
}
