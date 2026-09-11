import 'package:aa_bridge/nav/geo.dart';
import 'package:aa_bridge/nav/rgf2.dart';
import 'package:aa_bridge/nav/router.dart';
import 'package:aa_bridge/nav/way_classes.dart';
import 'package:flutter_test/flutter_test.dart';

import 'rgf2_packer.dart';

int _cls(String name) => kWayClasses.indexOf(name);

/// ~111 m per 0.001° of latitude at these latitudes; lengths below are in
/// decimetres so they stay integers.
const int _hopDm = 1112;

/// Two ways from 0 to 3: a long residential street and a short footway.
///
///   0 --res 1112-- 1 --res 1112-- 2 --res 1112-- 3
///   0 -------------- footway 1500 -------------- 3
RouteGraph _detourGraph() => RouteGraph.parse(packGraph(
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
            lenDm: _hopDm,
            wayClass: _cls('residential'),
            points: const <(double, double)>[(50.000, 20.0), (50.001, 20.0)]),
        TestEdge(
            from: 1,
            to: 2,
            lenDm: _hopDm,
            wayClass: _cls('residential'),
            points: const <(double, double)>[(50.001, 20.0), (50.002, 20.0)]),
        TestEdge(
            from: 2,
            to: 3,
            lenDm: _hopDm,
            wayClass: _cls('residential'),
            points: const <(double, double)>[(50.002, 20.0), (50.003, 20.0)]),
        TestEdge(
            from: 0,
            to: 3,
            lenDm: 1500,
            wayClass: _cls('footway'),
            points: const <(double, double)>[
              (50.0000, 20.0000),
              (50.0015, 20.0006),
              (50.0030, 20.0000),
            ]),
      ],
    ));

void main() {
  const start = LatLon(50.0, 20.0);
  const goal = LatLon(50.003, 20.0);

  test('picks the faster class over the shorter way', () async {
    final r = Router(_detourGraph());
    // Bicycle: residential 15 km/h over 333.6 m beats footway 5 km/h over 150 m.
    final bike = await r.route(start, goal, RideProfile.bicycle);
    expect(bike, isNotNull);
    expect(bike!.legs.length, 3);
    expect(bike.legs.every((l) => l.wayClass == _cls('residential')), isTrue);
  });

  test('a profile that likes pavement takes the short cut', () async {
    final r = Router(_detourGraph());
    // Scooter rides footway at 12 km/h, so 150 m of it wins.
    final scooter = await r.route(start, goal, RideProfile.scooter);
    expect(scooter!.legs.length, 1);
    expect(scooter.legs.single.wayClass, _cls('footway'));
    expect(scooter.lengthM, closeTo(150, 0.01));
  });

  test('reported length is the sum of the legs', () async {
    final r = Router(_detourGraph());
    final res = (await r.route(start, goal, RideProfile.bicycle))!;
    final sum = res.legs.fold<int>(0, (a, l) => a + l.lenDm) / 10.0;
    expect(res.lengthM, closeTo(sum, 1e-9));
    expect(res.timeS, closeTo(res.lengthM / (15 / 3.6), 1e-6));
  });

  test('glued geometry runs start to finish without repeating the seam',
      () async {
    final r = Router(_detourGraph());
    final res = (await r.route(start, goal, RideProfile.bicycle))!;
    expect(res.points.first.lat, closeTo(50.0, 1e-9));
    expect(res.points.last.lat, closeTo(50.003, 1e-9));
    expect(res.points.length, 4); // 3 edges × 2 points, seams dropped
    for (var i = 1; i < res.points.length; i++) {
      expect(res.points[i].lat, greaterThan(res.points[i - 1].lat));
    }
  });

  test('footSharePct reflects the classes ridden', () async {
    final r = Router(_detourGraph());
    expect((await r.route(start, goal, RideProfile.bicycle))!.footSharePct,
        closeTo(0, 1e-9));
    expect((await r.route(start, goal, RideProfile.scooter))!.footSharePct,
        closeTo(100, 1e-9));
  });

  test('a one-way edge is not driven backwards', () async {
    final g = RouteGraph.parse(packGraph(
      <(double, double)>[(50.0, 20.0), (50.001, 20.0)],
      <TestEdge>[
        TestEdge(
            from: 0,
            to: 1,
            lenDm: _hopDm,
            wayClass: _cls('residential'),
            bidir: false,
            points: const <(double, double)>[(50.0, 20.0), (50.001, 20.0)]),
      ],
    ));
    final r = Router(g);
    expect(await r.route(start, const LatLon(50.001, 20.0),
        RideProfile.bicycle), isNotNull);
    expect(
        await r.route(const LatLon(50.001, 20.0), start, RideProfile.bicycle),
        isNull);
  });

  test('a class the profile forbids is never used', () async {
    final g = RouteGraph.parse(packGraph(
      <(double, double)>[(50.0, 20.0), (50.001, 20.0)],
      <TestEdge>[
        TestEdge(
            from: 0,
            to: 1,
            lenDm: _hopDm,
            wayClass: _cls('steps'),
            points: const <(double, double)>[(50.0, 20.0), (50.001, 20.0)]),
      ],
    ));
    final r = Router(g);
    expect(
        await r.route(start, const LatLon(50.001, 20.0), RideProfile.bicycle),
        isNotNull);
    // Scooter cannot use steps, so both ends fall outside its usable graph.
    expect(
        await r.route(start, const LatLon(50.001, 20.0), RideProfile.scooter),
        isNull);
  });

  test('same start and goal, or nothing to snap to, gives no route', () async {
    final r = Router(_detourGraph());
    expect(await r.route(start, start, RideProfile.bicycle), isNull);
    expect(await r.route(const LatLon(10, 10), goal, RideProfile.bicycle),
        isNull);
  });
}
