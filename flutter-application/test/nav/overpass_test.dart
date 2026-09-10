/// Parsing what Overpass actually answers, and refusing what it should not be
/// asked. The network itself is exercised on a device; this pins the shapes.
library;

import 'package:aa_bridge/nav/osm_ways.dart';
import 'package:aa_bridge/nav/overpass.dart';
import 'package:aa_bridge/nav/way_classes.dart';
import 'package:flutter_test/flutter_test.dart';

const String kBody = '''
{"version": 0.6, "elements": [
  {"type": "way", "id": 1, "nodes": [10, 11, 12],
   "tags": {"highway": "residential", "name": "Floriańska"}},
  {"type": "way", "id": 2, "nodes": [12, 13],
   "tags": {"highway": "footway", "footway": "sidewalk"}},
  {"type": "way", "id": 3, "nodes": [20, 21],
   "tags": {"highway": "motorway"}},
  {"type": "way", "id": 4, "nodes": [30],
   "tags": {"highway": "residential"}},
  {"type": "node", "id": 10, "lat": 50.0, "lon": 20.0},
  {"type": "node", "id": 11, "lat": 50.001, "lon": 20.0},
  {"type": "node", "id": 12, "lat": 50.002, "lon": 20.0},
  {"type": "node", "id": 13, "lat": 50.003, "lon": 20.0},
  {"type": "node", "id": 99, "lat": 50.0, "lon": 20.001,
   "tags": {"name": "Kawiarnia", "amenity": "cafe"}}
]}
''';

void main() {
  group('GeoBounds', () {
    test('area shrinks with latitude', () {
      const equator = GeoBounds(south: 0, west: 0, north: 0.1, east: 0.1);
      const north = GeoBounds(south: 60, west: 0, north: 60.1, east: 0.1);
      expect(north.areaKm2, lessThan(equator.areaKm2 * 0.6));
    });

    test('a city-sized box is well inside the limit', () {
      // Roughly 11 x 7 km around Kraków.
      const krakow =
          GeoBounds(south: 50.02, west: 19.88, north: 50.09, east: 20.03);
      expect(krakow.areaKm2, lessThan(kMaxAreaKm2));
    });
  });

  group('buildQuery', () {
    test('asks for roads and named places in the box, with their nodes', () {
      final q = OverpassClient.buildQuery(
          const GeoBounds(south: 50.0, west: 19.9, north: 50.1, east: 20.0));
      expect(q, contains('[out:json]'));
      expect(q, contains('way["highway"](50.0,19.9,50.1,20.0)'));
      expect(q, contains('node["amenity"]'));
      // The recurse-down plus skel is what brings node coordinates back.
      expect(q, contains('>;'));
      expect(q, contains('out skel qt;'));
    });
  });

  group('parse', () {
    test('keeps routable ways and drops the rest', () {
      final r = OverpassClient.parse(kBody, kBody.length);
      expect(r.ways, hasLength(2), reason: 'motorway and the stub way go');
      expect(r.ways.first.wayClass, kWayClasses.indexOf('residential'));
      expect(r.ways.last.wayClass, kWayClasses.indexOf('sidewalk'));
      expect(r.ways.first.direction, WayDirection.both);
    });

    test('collects node coordinates', () {
      final r = OverpassClient.parse(kBody, kBody.length);
      expect(r.nodes, hasLength(5));
      expect(r.nodes[12]!.lat, closeTo(50.002, 1e-9));
    });

    test('collects named places for search', () {
      final r = OverpassClient.parse(kBody, kBody.length);
      expect(r.places, hasLength(1));
      expect(r.places.single.display, 'Kawiarnia');
      expect(r.places.single.kind, 'cafe');
    });

    test('reports the response size', () {
      expect(OverpassClient.parse(kBody, 4242).bytes, 4242);
    });

    test('anything that is not an Overpass answer is reported as such', () {
      for (final bad in <String>['nope', '[]', '{"x":1}', '{"elements":5}']) {
        expect(
            () => OverpassClient.parse(bad, 0),
            throwsA(isA<OverpassException>().having(
                (e) => e.messageKey, 'key', 'mapdata.err.overpassFormat')),
            reason: bad);
      }
    });

    test('a garbled element is skipped, the rest survive', () {
      const messy = '{"elements": ['
          '"not an object",'
          '{"type": "node", "id": 1},'
          '{"type": "way", "id": 2, "tags": {"highway": "residential"}},'
          '{"type": "node", "id": 3, "lat": 50.0, "lon": 20.0}'
          ']}';
      final r = OverpassClient.parse(messy, 0);
      expect(r.nodes, hasLength(1));
      expect(r.ways, isEmpty, reason: 'the way has no node list');
    });
  });

  test('an oversized area never reaches the network', () async {
    await expectLater(
      OverpassClient()
          .fetch(const GeoBounds(south: 40, west: 10, north: 55, east: 30)),
      throwsA(isA<OverpassException>()
          .having((e) => e.messageKey, 'key', 'mapdata.err.areaTooBig')),
    );
  });
}
