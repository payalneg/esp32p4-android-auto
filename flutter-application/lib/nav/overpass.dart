/// Fetches raw OSM data for an area straight from an Overpass server.
///
/// Overpass is the OSM project's own query API: ask for the roads inside a
/// bounding box and it answers with the ways and the nodes they reference —
/// exactly the two things graph_builder.dart needs. Compared with downloading
/// a regional `.osm.pbf` extract this is a few megabytes rather than a few
/// hundred, and it needs no PBF decoder on the phone.
///
/// The public instances are donated capacity, so this asks for one bounded
/// area at a time, names itself in the User-Agent and gives up rather than
/// retrying a refusal.
library;

import 'dart:async';
import 'dart:convert';
import 'dart:io';
import 'dart:math' as math;

import 'geo.dart';
import 'graph_builder.dart';
import 'osm_ways.dart';

/// Public Overpass instances, tried in order.
const List<String> kOverpassEndpoints = <String>[
  'https://overpass-api.de/api/interpreter',
  'https://overpass.kumi.systems/api/interpreter',
];

/// Above this the query is refused before it is sent: a bigger area answers
/// slowly, weighs tens of megabytes and rarely fits a ride anyway.
const double kMaxAreaKm2 = 900;

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

  Future<OverpassResult> fetch(
    GeoBounds bounds, {
    String? userAgent,
    void Function(int bytes)? onProgress,
  }) async {
    if (bounds.areaKm2 > kMaxAreaKm2) {
      throw OverpassException('mapdata.err.areaTooBig', <String, String>{
        'km2': bounds.areaKm2.round().toString(),
        'max': kMaxAreaKm2.round().toString(),
      });
    }
    Object? lastError;
    for (final endpoint in _endpoints) {
      final client = _clientFactory();
      if (userAgent != null) client.userAgent = userAgent;
      try {
        final req = await client.postUrl(Uri.parse(endpoint));
        req.headers.contentType =
            ContentType('application', 'x-www-form-urlencoded', charset: 'utf-8');
        req.write('data=${Uri.encodeQueryComponent(buildQuery(bounds))}');
        final resp = await req.close().timeout(const Duration(minutes: 4));
        if (resp.statusCode == 429 || resp.statusCode == 504) {
          await resp.drain<void>();
          throw OverpassException('mapdata.err.overpassBusy');
        }
        if (resp.statusCode != 200) {
          await resp.drain<void>();
          throw OverpassException(
              'mapdata.err.http', <String, String>{'code': '${resp.statusCode}'});
        }
        final buffer = StringBuffer();
        var received = 0;
        await for (final chunk in resp.transform(utf8.decoder)) {
          buffer.write(chunk);
          received += chunk.length;
          onProgress?.call(received);
        }
        return parse(buffer.toString(), received);
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
  static OverpassResult parse(String body, int bytes) {
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
