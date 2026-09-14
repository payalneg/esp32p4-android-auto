import 'dart:math' as math;

import 'package:aa_bridge/nav/geo.dart';
import 'package:aa_bridge/nav/maneuvers.dart';
import 'package:aa_bridge/nav/route_guide.dart';
import 'package:aa_bridge/nav/router.dart';
import 'package:aa_bridge/nav/way_classes.dart';
import 'package:flutter_test/flutter_test.dart';

const double _mLat = 1 / 111320.0;
double _mLon(double lat) => 1 / (111320.0 * math.cos(lat * math.pi / 180));

/// A 1 km straight run north, then 1 km east — long enough for a maneuver.
RouteResult _lRoute() {
  const lat0 = 50.0, lon0 = 20.0;
  final dLon = _mLon(lat0);
  final pts = <LatLon>[];
  for (var d = 0.0; d <= 1000; d += 10) {
    pts.add(LatLon(lat0 + d * _mLat, lon0));
  }
  for (var d = 10.0; d <= 1000; d += 10) {
    pts.add(LatLon(lat0 + 1000 * _mLat, lon0 + d * dLon));
  }
  return RouteResult(
    points: pts,
    lengthM: cumulativeM(pts).last,
    timeS: 0,
    legs: const <({int wayClass, int lenDm})>[],
    profile: RideProfile.scooter,
  );
}

void main() {
  test('on the line: no offset, distance counts down', () {
    final g = RouteGuide(_lRoute());
    final first = g.update(g.positionAt(0));
    expect(first.offsetM, closeTo(0, 0.5));
    expect(first.remainingM, closeTo(g.totalM, 1));

    var previous = first.remainingM;
    for (var d = 25.0; d < g.totalM; d += 25) {
      final s = g.update(g.positionAt(d));
      expect(s.offsetM, closeTo(0, 0.5), reason: 'at $d m');
      expect(s.remainingM, lessThan(previous));
      previous = s.remainingM;
    }
  });

  test('announces the corner and counts down to it', () {
    final g = RouteGuide(_lRoute());
    final early = g.update(g.positionAt(100));
    expect(early.next.type, ManeuverType.turnRight);
    final later = g.update(g.positionAt(500));
    expect(later.next.type, ManeuverType.turnRight);
    expect(later.nextDistM, lessThan(early.nextDistM));
    expect(later.nextDistM, closeTo(500, 20));
  });

  test('after the corner the next maneuver is the arrival', () {
    final g = RouteGuide(_lRoute());
    final s = g.update(g.positionAt(1200));
    expect(s.next.type, ManeuverType.arrive);
  });

  test('arrival is flagged near the end', () {
    final g = RouteGuide(_lRoute());
    expect(g.update(g.positionAt(g.totalM - 100)).arrived, isFalse);
    expect(g.update(g.positionAt(g.totalM)).arrived, isTrue);
  });

  test('leaving the route needs three fixes, returning clears it', () {
    final g = RouteGuide(_lRoute());
    g.update(g.positionAt(500));
    final away = LatLon(g.positionAt(500).lat, 20.0 + 120 * _mLon(50.0));

    expect(g.update(away).offRoute, isFalse); // 1
    expect(g.update(away).offRoute, isFalse); // 2
    final third = g.update(away);
    expect(third.offRoute, isTrue); // 3
    expect(third.offsetM, closeTo(120, 10));

    expect(g.update(g.positionAt(520)).offRoute, isFalse);
  });

  test('a wobble inside the hysteresis band holds the current state', () {
    final g = RouteGuide(_lRoute());
    g.update(g.positionAt(500));
    // 20 m out: past "back on" but short of "off route" — nothing changes.
    final wobble = LatLon(g.positionAt(500).lat, 20.0 + 20 * _mLon(50.0));
    for (var i = 0; i < 5; i++) {
      expect(g.update(wobble).offRoute, isFalse);
    }
  });

  test('a teleport ahead is matched globally, not stuck at the window edge',
      () {
    final g = RouteGuide(_lRoute());
    g.update(g.positionAt(0));
    // 1500 m along is far outside the +300 m search window.
    final s = g.update(g.positionAt(1500));
    expect(s.alongM, closeTo(1500, 20));
    expect(s.offsetM, closeTo(0, 1));
  });

  test('positionAt round-trips against the cumulative table', () {
    final g = RouteGuide(_lRoute());
    for (final d in <double>[0, 250, 999, 1500, 1990]) {
      final p = g.positionAt(d);
      expect(g.update(p).alongM, closeTo(d, 1));
    }
  });
}
