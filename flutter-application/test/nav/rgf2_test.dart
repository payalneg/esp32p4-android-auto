import 'dart:typed_data';

import 'package:aa_bridge/nav/geo.dart';
import 'package:aa_bridge/nav/rgf2.dart';
import 'package:aa_bridge/nav/way_classes.dart';
import 'package:flutter_test/flutter_test.dart';

import 'rgf2_packer.dart';

int _cls(String name) => kWayClasses.indexOf(name);

/// A short street with a flight of steps hanging off its far end:
///
///   0 --res-- 1 --res(one-way)--> 2 --steps-- 3
///
/// Node 3 is reachable on a bicycle (steps allowed, if slow) but not on a
/// scooter, which is the profile difference the snapping has to respect.
Uint8List _sample() => packGraph(
      <(double, double)>[
        (50.000, 20.000),
        (50.001, 20.000),
        (50.002, 20.000),
        (50.003, 20.000),
      ],
      <TestEdge>[
        TestEdge(
          from: 0,
          to: 1,
          lenDm: 1112,
          wayClass: _cls('residential'),
          points: <(double, double)>[
            (50.0000, 20.0),
            (50.0005, 20.0),
            (50.0010, 20.0),
          ],
        ),
        TestEdge(
          from: 1,
          to: 2,
          lenDm: 1112,
          wayClass: _cls('residential'),
          bidir: false, // one-way
          points: <(double, double)>[(50.001, 20.0), (50.002, 20.0)],
        ),
        TestEdge(
          from: 2,
          to: 3,
          lenDm: 1112,
          wayClass: _cls('steps'),
          points: <(double, double)>[(50.002, 20.0), (50.003, 20.0)],
        ),
      ],
    );

void main() {
  group('parse', () {
    test('reads counts, nodes and edge fields', () {
      final g = RouteGraph.parse(_sample());
      expect(g.nodeCount, 4);
      expect(g.edgeCount, 3);
      expect(g.node(1).lat, closeTo(50.001, 1e-9));
      expect(g.edgeFrom[1], 1);
      expect(g.edgeTo[1], 2);
      expect(g.edgeLenDm[0], 1112);
      expect(g.edgeClass[2], _cls('steps'));
    });

    test('rejects a wrong signature', () {
      expect(() => RouteGraph.parse(packGraph(const [], const [], magic: 1)),
          throwsFormatException);
    });

    test('rejects a truncated file', () {
      final full = _sample();
      expect(() => RouteGraph.parse(full.sublist(0, full.length - 9)),
          throwsFormatException);
      expect(() => RouteGraph.parse(Uint8List(6)), throwsFormatException);
    });

    test('rejects an unknown way class', () {
      final bad = packGraph(
        <(double, double)>[(50.0, 20.0), (50.001, 20.0)],
        <TestEdge>[
          TestEdge(
            from: 0,
            to: 1,
            lenDm: 100,
            wayClass: 99,
            points: <(double, double)>[(50.0, 20.0), (50.001, 20.0)],
          ),
        ],
      );
      expect(() => RouteGraph.parse(bad), throwsFormatException);
    });
  });

  group('geometry', () {
    test('is stored per edge and read back in order', () {
      final g = RouteGraph.parse(_sample());
      final fwd = g.edgePoints(0, true);
      expect(fwd.length, 3);
      expect(fwd.first.lat, closeTo(50.0, 1e-9));
      expect(fwd.last.lat, closeTo(50.001, 1e-9));
      final rev = g.edgePoints(0, false);
      expect(rev.first.lat, closeTo(50.001, 1e-9));
      expect(rev.last.lat, closeTo(50.0, 1e-9));
    });
  });

  group('CSR adjacency', () {
    test('a two-way edge appears at both ends, a one-way only at its start',
        () {
      final g = RouteGraph.parse(_sample());
      int degree(int node) => g.adjStart[node + 1] - g.adjStart[node];
      expect(degree(0), 1); // edge 0 forward
      expect(degree(1), 2); // edge 0 reversed + edge 1 forward
      expect(degree(2), 1); // edge 1 arrives one-way; edge 2 leaves
      expect(degree(3), 1); // edge 2 reversed
    });

    test('slots know their direction and target', () {
      final g = RouteGraph.parse(_sample());
      final slot = g.adjStart[0];
      expect(g.adjEdgeIndex(slot), 0);
      expect(g.adjForward(slot), isTrue);
      expect(g.adjTarget(slot), 1);
      final back = g.adjStart[1];
      final reverse = <int>[
        for (var s = back; s < g.adjStart[2]; s++)
          if (!g.adjForward(s)) g.adjTarget(s)
      ];
      expect(reverse, <int>[0]);
    });
  });

  group('nearestNode', () {
    test('snaps to the closest vertex', () {
      final g = RouteGraph.parse(_sample());
      expect(g.nearestNode(const LatLon(50.0009, 20.0)), 1);
    });

    test('gives up past the snap radius', () {
      final g = RouteGraph.parse(_sample());
      expect(g.nearestNode(const LatLon(51.0, 21.0)), isNull);
    });

    test('a profile snaps past what it cannot ride', () {
      final g = RouteGraph.parse(_sample());
      const nearTheSteps = LatLon(50.0029, 20.0);
      // Plain nearest: the top of the steps.
      expect(g.nearestNode(nearTheSteps), 3);
      // A bicycle may use steps, so that vertex stays on offer.
      expect(g.nearestNode(nearTheSteps, profile: RideProfile.bicycle), 3);
      // A scooter cannot, so the tap falls back to the street below instead of
      // producing a route that can never be found.
      expect(g.nearestNode(nearTheSteps, profile: RideProfile.scooter), 2);
    });
  });
}
