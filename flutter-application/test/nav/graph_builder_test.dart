/// The phone builds its own graph now, so the rules that used to live only in
/// builder.py have to hold here too — a road classified differently on the
/// phone would route differently from the same city built on a desktop.
library;

import 'package:aa_bridge/nav/geo.dart';
import 'package:aa_bridge/nav/graph_builder.dart';
import 'package:aa_bridge/nav/osm_ways.dart';
import 'package:aa_bridge/nav/rgf2.dart';
import 'package:aa_bridge/nav/way_classes.dart';
import 'package:flutter_test/flutter_test.dart';

int cls(String name) => kWayClasses.indexOf(name);

void main() {
  group('classifyWay', () {
    test('maps the highway types the generator maps', () {
      expect(classifyWay({'highway': 'residential'}), cls('residential'));
      expect(classifyWay({'highway': 'living_street'}), cls('residential'));
      expect(classifyWay({'highway': 'secondary_link'}), cls('secondary'));
      expect(classifyWay({'highway': 'steps'}), cls('steps'));
    });

    test('ignores what is not part of the network', () {
      expect(classifyWay({'highway': 'motorway'}), isNull);
      expect(classifyWay({'building': 'yes'}), isNull);
      expect(classifyWay(const <String, String>{}), isNull);
    });

    test('a pavement and a crossing get their own class', () {
      expect(classifyWay({'highway': 'footway', 'footway': 'sidewalk'}),
          cls('sidewalk'));
      expect(classifyWay({'highway': 'footway', 'footway': 'crossing'}),
          cls('crossing'));
      expect(classifyWay({'highway': 'path', 'path': 'crossing'}),
          cls('crossing'));
      expect(classifyWay({'highway': 'footway'}), cls('footway'));
    });

    test('a path you may ride counts as a cycleway', () {
      expect(classifyWay({'highway': 'footway', 'bicycle': 'designated'}),
          cls('cycleway'));
      expect(
          classifyWay({
            'highway': 'footway',
            'footway': 'sidewalk',
            'bicycle': 'yes',
          }),
          cls('cycleway'));
      expect(classifyWay({'highway': 'pedestrian', 'bicycle': 'yes'}),
          cls('cycleway'));
    });

    test('private ground is dropped unless cycling is allowed', () {
      expect(classifyWay({'highway': 'service', 'access': 'private'}), isNull);
      expect(classifyWay({'highway': 'service', 'access': 'no'}), isNull);
      expect(
          classifyWay(
              {'highway': 'service', 'access': 'private', 'bicycle': 'yes'}),
          cls('service'));
    });

    test('bicycle=no keeps the road — the profile decides, not the builder',
        () {
      expect(classifyWay({'highway': 'primary', 'bicycle': 'no'}),
          cls('primary'));
      expect(classifyWay({'highway': 'primary', 'bicycle': 'use_sidepath'}),
          cls('primary'));
    });
  });

  group('wayDirection', () {
    test('plain streets are two-way', () {
      expect(wayDirection(const <String, String>{}), WayDirection.both);
      expect(wayDirection({'oneway': 'no'}), WayDirection.both);
    });

    test('one-way in both spellings', () {
      for (final v in <String>['yes', '1', 'true']) {
        expect(wayDirection({'oneway': v}), WayDirection.forward, reason: v);
      }
      expect(wayDirection({'oneway': '-1'}), WayDirection.backward);
      expect(wayDirection({'oneway:bicycle': 'yes'}), WayDirection.forward);
    });

    test('a contraflow lane makes a one-way street two-way again', () {
      expect(wayDirection({'oneway': 'yes', 'oneway:bicycle': 'no'}),
          WayDirection.both);
      expect(
          wayDirection({'oneway': 'yes', 'cycleway:left': 'opposite_lane'}),
          WayDirection.both);
      expect(wayDirection({'oneway': 'yes', 'cycleway': 'opposite'}),
          WayDirection.both);
    });
  });

  group('searchEntry', () {
    test('an address needs both street and number', () {
      expect(
          searchEntry({'addr:street': 'Floriańska', 'addr:housenumber': '12'})
              ?.display,
          'Floriańska 12');
      expect(searchEntry({'addr:street': 'Floriańska'}), isNull);
    });

    test('a named POI needs a category', () {
      expect(searchEntry({'name': 'Kawiarnia', 'amenity': 'cafe'})?.kind,
          'cafe');
      expect(searchEntry({'name': 'Nowhere'}), isNull);
    });
  });

  group('GraphBuilder', () {
    /// Two streets crossing at a shared node:
    ///
    ///   1 --- 2 --- 3      (west to east, ids 1..3)
    ///         |
    ///         4            (south, way B is 2--4)
    final nodes = <int, LatLon>{
      1: const LatLon(50.000, 20.000),
      2: const LatLon(50.000, 20.002),
      3: const LatLon(50.000, 20.004),
      4: const LatLon(49.998, 20.002),
    };

    test('cuts a way at the junction and keeps the geometry between', () {
      final built = GraphBuilder.build(
        ways: <OsmWay>[
          OsmWay(
              wayClass: cls('residential'),
              direction: WayDirection.both,
              nodeIds: const <int>[1, 2, 3]),
          OsmWay(
              wayClass: cls('residential'),
              direction: WayDirection.both,
              nodeIds: const <int>[2, 4]),
        ],
        nodes: nodes,
      );
      // 1-2, 2-3 and 2-4: the through street is split at the crossing.
      expect(built.edgeCount, 3);
      expect(built.nodeCount, 4);

      final graph = RouteGraph.parse(built.graphBytes);
      expect(graph.nodeCount, 4);
      expect(graph.edgeCount, 3);
      for (var e = 0; e < graph.edgeCount; e++) {
        expect(graph.edgePoints(e, true).length, 2);
        expect(graph.edgeLenDm[e], greaterThan(0));
      }
    });

    test('an untouched middle node stays geometry, not a vertex', () {
      final built = GraphBuilder.build(
        ways: <OsmWay>[
          OsmWay(
              wayClass: cls('residential'),
              direction: WayDirection.both,
              nodeIds: const <int>[1, 2, 3]),
        ],
        nodes: nodes,
      );
      expect(built.nodeCount, 2, reason: 'only the two ends');
      expect(built.edgeCount, 1);
      final graph = RouteGraph.parse(built.graphBytes);
      expect(graph.edgePoints(0, true).length, 3, reason: 'the bend is kept');
    });

    test('direction reaches the flags, and -1 reverses the edge', () {
      final forward = GraphBuilder.build(
        ways: <OsmWay>[
          OsmWay(
              wayClass: cls('residential'),
              direction: WayDirection.forward,
              nodeIds: const <int>[1, 2]),
        ],
        nodes: nodes,
      );
      expect(RouteGraph.parse(forward.graphBytes).edgeFlags[0] & kFlagBidir, 0);

      final backward = GraphBuilder.build(
        ways: <OsmWay>[
          OsmWay(
              wayClass: cls('residential'),
              direction: WayDirection.backward,
              nodeIds: const <int>[1, 2]),
        ],
        nodes: nodes,
      );
      final g = RouteGraph.parse(backward.graphBytes);
      // Reversed: the edge now starts at node 2, which sits further east.
      expect(g.node(g.edgeFrom[0]).lon, closeTo(20.002, 1e-6));
    });

    test('an island is dropped, the mainland survives', () {
      final built = GraphBuilder.build(
        ways: <OsmWay>[
          OsmWay(
              wayClass: cls('residential'),
              direction: WayDirection.both,
              nodeIds: const <int>[1, 2]),
          OsmWay(
              wayClass: cls('residential'),
              direction: WayDirection.both,
              nodeIds: const <int>[2, 3]),
          // Detached pair, far away.
          OsmWay(
              wayClass: cls('service'),
              direction: WayDirection.both,
              nodeIds: const <int>[10, 11]),
        ],
        nodes: <int, LatLon>{
          ...nodes,
          10: const LatLon(51.0, 21.0),
          11: const LatLon(51.001, 21.0),
        },
      );
      expect(built.edgeCount, 2);
      expect(built.nodeCount, 3);
    });

    test('ways with missing geometry are skipped, not guessed at', () {
      final built = GraphBuilder.build(
        ways: <OsmWay>[
          OsmWay(
              wayClass: cls('residential'),
              direction: WayDirection.both,
              nodeIds: const <int>[1, 2]),
          OsmWay(
              wayClass: cls('residential'),
              direction: WayDirection.both,
              nodeIds: const <int>[2, 999]), // 999 was clipped by the bbox
        ],
        nodes: nodes,
      );
      expect(built.edgeCount, 1);
    });

    test('an empty area builds an empty graph rather than throwing', () {
      final built =
          GraphBuilder.build(ways: const <OsmWay>[], nodes: <int, LatLon>{});
      expect(built.edgeCount, 0);
      expect(built.nodeCount, 0);
      expect(RouteGraph.parse(built.graphBytes).edgeCount, 0);
    });

    test('places become a search index the reader understands', () {
      final built = GraphBuilder.build(
        ways: <OsmWay>[
          OsmWay(
              wayClass: cls('residential'),
              direction: WayDirection.both,
              nodeIds: const <int>[1, 2]),
        ],
        nodes: nodes,
        places: <OsmPlace>[
          const OsmPlace('Floriańska 12', 'address', LatLon(50.0, 20.0)),
          const OsmPlace('Kawiarnia Florian', 'cafe', LatLon(50.001, 20.001)),
          const OsmPlace('Floriańska 12', 'address', LatLon(50.0, 20.0)),
        ],
      );
      expect(built.placeCount, 2, reason: 'the duplicate is folded away');
      final lines = built.searchIndexTsv.trim().split('\n');
      expect(lines.first.split('\t').first, startsWith('florianska'));
      expect(lines.first.split('\t').length, 5);
    });
  });
}
