/// When the app is allowed to ask for a map it does not have.
///
/// The interesting half of the feature is restraint: a rider going about a
/// city should be asked once, not warned at every junction.
library;

import 'dart:convert';
import 'dart:io';

import 'package:aa_bridge/nav/geofabrik.dart';
import 'package:aa_bridge/nav/geo.dart';
import 'package:aa_bridge/nav/map_data.dart';
import 'package:aa_bridge/nav/region_suggester.dart';
import 'package:aa_bridge/nav/rgf2.dart';
import 'package:aa_bridge/nav/way_classes.dart';
import 'package:aa_bridge/settings/nav_settings.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:shared_preferences/shared_preferences.dart';

import 'rgf2_packer.dart';

const LatLon kHere = LatLon(50.0619, 19.9368);

/// A two-vertex street at [lat] degrees north.
RouteGraph _graphAt(double lat) => RouteGraph.parse(packGraph(
      <(double, double)>[
        (lat, 19.9368),
        (lat + 0.001, 19.9368),
      ],
      <TestEdge>[
        TestEdge(
          from: 0,
          to: 1,
          lenDm: 111,
          wayClass: kWayClasses.indexOf('residential'),
          points: <(double, double)>[
            (lat, 19.9368),
            (lat + 0.001, 19.9368),
          ],
        ),
      ],
    ));

/// A two-vertex street in the middle of Krakow.
RouteGraph _graph() => _graphAt(50.0619);

/// Poland with Lesser Poland inside it, enough to exercise the lookup.
const String _catalogueJson = '''
{"features": [
  {"properties": {"id": "poland", "name": "Poland",
    "urls": {"pbf": "https://example.org/pl.osm.pbf"}},
   "geometry": {"type": "Polygon",
     "coordinates": [[[18,49],[22,49],[22,52],[18,52],[18,49]]]}},
  {"properties": {"id": "malopolskie", "name": "Malopolskie",
    "parent": "poland", "urls": {"pbf": "https://example.org/mal.osm.pbf"}},
   "geometry": {"type": "MultiPolygon", "coordinates": [[[[19.5,49.8],
     [20.5,49.8],[20.5,50.5],[19.5,50.5],[19.5,49.8]]]]}}
]}
''';

void main() {
  setUp(() {
    SharedPreferences.setMockInitialValues(<String, Object>{});
  });

  group('MapData.covers', () {
    test('is true where the graph has a road', () {
      expect(MapData.forTest(_graph()).covers(kHere), isTrue);
    });

    test('stays true just outside the envelope, within the snap margin', () {
      // ~100 m north of the northern vertex.
      expect(
          MapData.forTest(_graph()).covers(const LatLon(50.0638, 19.9368)),
          isTrue);
    });

    test('is false once the point leaves the envelope', () {
      expect(MapData.forTest(_graph()).covers(const LatLon(50.20, 19.9368)),
          isFalse);
      expect(MapData.forTest(_graph()).covers(const LatLon(51.5, 20.0)),
          isFalse);
    });

    test('is false without a graph', () {
      final empty = RouteGraph.parse(packGraph(const [], const []));
      expect(MapData.forTest(empty).covers(kHere), isFalse);
    });
  });

  group('RegionSuggester', () {
    test('offers the region for a position the graph does not cover', () async {
      final suggester = RegionSuggester(
        map: MapData.forTest(_graph()),
        settings: NavSettings.forTest(),
        loadCatalogue: () async => GeofabrikIndex.parse(_catalogueJson),
      );
      final offer = await suggester.check(const LatLon(51.5, 20.0));
      expect(offer?.region.id, 'poland');
      expect(offer?.label, 'Poland');
    });

    test('names the province when it is the more specific answer', () async {
      final suggester = RegionSuggester(
        map: MapData.forTest(_graph()),
        settings: NavSettings.forTest(),
        loadCatalogue: () async => GeofabrikIndex.parse(_catalogueJson),
      );
      // Inside Lesser Poland, but 20 km from the one street in the graph.
      final offer = await suggester.check(const LatLon(50.2, 19.7));
      expect(offer?.region.id, 'malopolskie');
      expect(offer?.label, 'Poland / Malopolskie');
    });

    test('says nothing where a downloaded map already covers the rider',
        () async {
      final suggester = RegionSuggester(
        map: MapData.forTest(_graph()),
        settings: NavSettings.forTest(),
        loadCatalogue: () async => GeofabrikIndex.parse(_catalogueJson),
      );
      expect(await suggester.check(kHere), isNull);
    });

    test('does not offer the region that is already downloaded', () async {
      final suggester = RegionSuggester(
        map: MapData.forTest(_graph(), regionId: 'malopolskie'),
        settings: NavSettings.forTest(),
        loadCatalogue: () async => GeofabrikIndex.parse(_catalogueJson),
      );
      expect(await suggester.check(const LatLon(50.2, 19.7)), isNull);
    });

    test('respects "Not now"', () async {
      final settings = NavSettings.forTest();
      await settings.snoozeRegion('malopolskie');
      final suggester = RegionSuggester(
        map: MapData.forTest(_graph()),
        settings: settings,
        loadCatalogue: () async => GeofabrikIndex.parse(_catalogueJson),
      );
      expect(await suggester.check(const LatLon(50.2, 19.7)), isNull);
    });

    test('respects the switch', () async {
      final settings = NavSettings.forTest();
      await settings.setAutoRegion(false);
      final suggester = RegionSuggester(
        map: MapData.forTest(_graph()),
        settings: settings,
        loadCatalogue: () async => GeofabrikIndex.parse(_catalogueJson),
      );
      expect(await suggester.check(const LatLon(51.5, 20.0)), isNull);
    });

    test('checks once per place, not once per fix', () async {
      var loads = 0;
      final suggester = RegionSuggester(
        map: MapData.forTest(_graph()),
        settings: NavSettings.forTest(),
        loadCatalogue: () async {
          loads++;
          return GeofabrikIndex.parse(_catalogueJson);
        },
      );
      expect(await suggester.check(const LatLon(51.5, 20.0)), isNotNull);
      expect(await suggester.check(const LatLon(51.51, 20.01)), isNull);
      expect(loads, 1);
      // A download or a settings change makes the old answer stale.
      suggester.reset();
      expect(await suggester.check(const LatLon(51.51, 20.01)), isNotNull);
    });

    test('a destination bypasses the position throttle', () async {
      final suggester = RegionSuggester(
        map: MapData.forTest(_graph()),
        settings: NavSettings.forTest(),
        loadCatalogue: () async => GeofabrikIndex.parse(_catalogueJson),
      );
      final here = await suggester.check(const LatLon(51.9, 20.0));
      expect(here?.reason, RegionSuggestionReason.here);
      // The same place again is throttled…
      expect(await suggester.check(const LatLon(51.9, 20.0)), isNull);
      // …but a destination is not: it is a different question.
      final there = await suggester.check(const LatLon(51.9, 20.0),
          oncePerPlace: false, reason: RegionSuggestionReason.destination);
      expect(there?.reason, RegionSuggestionReason.destination);
    });

    test('a failed lookup is silence, not an error', () async {
      final suggester = RegionSuggester(
        map: MapData.forTest(_graph()),
        settings: NavSettings.forTest(),
        loadCatalogue: () async =>
            throw GeofabrikException('mapdata.err.download'),
      );
      expect(await suggester.check(const LatLon(51.5, 20.0)), isNull);
    });

    test('the open sea has no region to offer', () async {
      final suggester = RegionSuggester(
        map: MapData.forTest(_graph()),
        settings: NavSettings.forTest(),
        loadCatalogue: () async => GeofabrikIndex.parse(_catalogueJson),
      );
      expect(await suggester.check(const LatLon(40.0, 0.0)), isNull);
    });

    test('a region already kept on the phone is offered as a switch', () async {
      final nav = await Directory.systemTemp.createTemp('navsuggest');
      addTearDown(() async {
        if (await nav.exists()) await nav.delete(recursive: true);
      });
      final graph = File('${nav.path}/${MapData.graphFileName}');
      final source = File('${nav.path}/${MapData.sourceFileName}');

      // Keep Poland as a saved map by briefly making it the active one.
      await graph.writeAsBytes(
          packGraph(
            <(double, double)>[(51.5, 20.0), (51.501, 20.0)],
            <TestEdge>[
              TestEdge(
                from: 0,
                to: 1,
                lenDm: 111,
                wayClass: kWayClasses.indexOf('residential'),
                points: <(double, double)>[(51.5, 20.0), (51.501, 20.0)],
              ),
            ],
          ),
          flush: true);
      await source.writeAsString(
          jsonEncode(<String, String>{'id': 'poland', 'name': 'Poland'}));
      final map = MapData.forTest(_graph(), navDir: nav);
      await map.reloadFromDisk();
      await map.saveActiveMap();

      // The rider's actual map is the one street in Krakow.
      await graph.writeAsBytes(packGraph(
        <(double, double)>[(50.0619, 19.9368), (50.0629, 19.9368)],
        <TestEdge>[
          TestEdge(
            from: 0,
            to: 1,
            lenDm: 111,
            wayClass: kWayClasses.indexOf('residential'),
            points: <(double, double)>[
              (50.0619, 19.9368),
              (50.0629, 19.9368),
            ],
          ),
        ],
      ), flush: true);
      await source.writeAsString(jsonEncode(<String, String>{
        'id': 'malopolskie',
        'name': 'Malopolskie',
      }));
      await map.reloadFromDisk();

      final suggester = RegionSuggester(
        map: map,
        settings: NavSettings.forTest(),
        loadCatalogue: () async => GeofabrikIndex.parse(_catalogueJson),
      );
      final offer = await suggester.check(const LatLon(51.0, 20.0));
      expect(offer?.region.id, 'poland');
      expect(offer?.saved, isTrue);
    });
  });
}

