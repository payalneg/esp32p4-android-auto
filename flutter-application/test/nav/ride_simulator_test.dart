/// The simulator is how the navigator is exercised without going outside, so
/// it has to produce fixes the guidance treats exactly like GPS ones.
library;

import 'dart:math' as math;

import 'package:aa_bridge/nav/geo.dart';
import 'package:aa_bridge/nav/location_service.dart';
import 'package:aa_bridge/nav/ride_simulator.dart';
import 'package:aa_bridge/nav/route_guide.dart';
import 'package:aa_bridge/nav/router.dart';
import 'package:aa_bridge/nav/way_classes.dart';
import 'package:flutter_test/flutter_test.dart';

const double _mLat = 1 / 111320.0;

RouteGuide _straightRoute(double metres) {
  final pts = <LatLon>[
    for (var d = 0.0; d <= metres; d += 10) LatLon(50.0 + d * _mLat, 20.0),
  ];
  return RouteGuide(RouteResult(
    points: pts,
    lengthM: cumulativeM(pts).last,
    timeS: 0,
    legs: const <({int wayClass, int lenDm})>[],
    profile: RideProfile.scooter,
  ));
}

void main() {
  test('delayed timer displacement agrees with advertised speed', () async {
    var elapsed = Duration.zero;
    var samples = 0;
    final fixes = await simulateRide(_straightRoute(100),
            speedMs: 5, speedup: 1, tick: const Duration(milliseconds: 1),
            elapsed: () => elapsed)
        .map((fix) {
          samples++;
          if (samples == 1) elapsed = const Duration(milliseconds: 500);
          if (samples == 2) elapsed = const Duration(milliseconds: 1500);
          return fix;
        })
        .take(3)
        .toList();
    final travelled = fixes.map((f) => (f.position.lat - 50) / _mLat).toList();
    expect(travelled[0], closeTo(0, .02));
    expect(travelled[1], closeTo(2.5, .02));
    expect(travelled[2], closeTo(7.5, .02));
    expect(fixes.map((f) => f.speedMs), everyElement(5));
  });

  test('walks the route and closes at the end', () async {
    final guide = _straightRoute(300);
    final fixes = await simulateRide(guide,
            tick: const Duration(milliseconds: 1), speedup: 100)
        .toList();

    expect(fixes, isNotEmpty);
    expect(fixes.first.position.lat, closeTo(50.0, 1e-4));
    expect(fixes.last.position.lat,
        closeTo(guide.points.last.lat, 1e-6),
        reason: 'the last fix sits at the destination');
  });

  test('fixes move forward and carry a heading', () async {
    final guide = _straightRoute(200);
    final fixes = await simulateRide(guide,
            tick: const Duration(milliseconds: 1), speedup: 50)
        .toList();

    for (var i = 1; i < fixes.length; i++) {
      expect(fixes[i].position.lat,
          greaterThanOrEqualTo(fixes[i - 1].position.lat));
    }
    // Due north, so every heading should be about zero.
    for (final f in fixes.skip(1)) {
      final h = f.headingDeg!;
      expect(math.min(h, 360 - h), lessThan(5));
    }
    expect(fixes.skip(1).take(fixes.length - 2).map((f) => f.speedMs),
        everyElement(greaterThan(0)));
    expect(fixes.last.speedMs, 0,
        reason: 'the head unit must not predict past the simulated destination');
  });

  test('guidance consumes simulated fixes exactly like real ones', () async {
    final guide = _straightRoute(500);
    var last = double.infinity;
    await for (final GeoFix fix in simulateRide(guide,
        tick: const Duration(milliseconds: 1), speedup: 50)) {
      final g = guide.update(fix.position);
      expect(g.offsetM, closeTo(0, 1), reason: 'a simulated rider is on the line');
      expect(g.remainingM, lessThanOrEqualTo(last + 1e-6));
      last = g.remainingM;
    }
    expect(last, lessThan(kArriveM));
  });

  test('cancelling the subscription stops the timer', () async {
    final guide = _straightRoute(10000);
    final sub = simulateRide(guide, tick: const Duration(milliseconds: 1))
        .listen((_) {});
    await Future<void>.delayed(const Duration(milliseconds: 20));
    await sub.cancel();
    // Nothing to assert beyond "this returns": a leaked periodic timer would
    // keep the test binding alive and fail the run.
  });
}
