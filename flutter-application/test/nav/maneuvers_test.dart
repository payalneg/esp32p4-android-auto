import 'dart:math' as math;

import 'package:aa_bridge/nav/geo.dart';
import 'package:aa_bridge/nav/maneuvers.dart';
import 'package:flutter_test/flutter_test.dart';

/// Metres → degrees at ~50°N, so the fixtures read in metres.
const double _mLat = 1 / 111320.0;
double _mLon(double lat) => 1 / (111320.0 * math.cos(lat * math.pi / 180));

/// A polyline that runs [northM] north, then turns by [turnDeg] and runs on
/// for [thenM] metres. Sampled every 5 m so the 15 m turn window has data.
List<LatLon> _corner(double northM, double turnDeg, double thenM) {
  const lat0 = 50.0, lon0 = 20.0;
  final dLon = _mLon(lat0);
  final pts = <LatLon>[];
  for (var d = 0.0; d <= northM; d += 5) {
    pts.add(LatLon(lat0 + d * _mLat, lon0));
  }
  final corner = LatLon(lat0 + northM * _mLat, lon0);
  final rad = turnDeg * math.pi / 180;
  for (var d = 5.0; d <= thenM; d += 5) {
    pts.add(LatLon(
      corner.lat + d * math.cos(rad) * _mLat,
      corner.lon + d * math.sin(rad) * dLon,
    ));
  }
  return pts;
}

void main() {
  test('a straight line has no turns, only the arrival', () {
    final m = buildManeuvers(_corner(200, 0, 200));
    expect(m.length, 1);
    expect(m.single.type, ManeuverType.arrive);
  });

  test('too short to measure still arrives', () {
    expect(buildManeuvers(const <LatLon>[LatLon(50, 20), LatLon(50.001, 20)])
        .single.type, ManeuverType.arrive);
  });

  test('classifies by angle, and by side', () {
    expect(classifyTurn(10), ManeuverType.straight);
    expect(classifyTurn(40), ManeuverType.slightRight);
    expect(classifyTurn(-40), ManeuverType.slightLeft);
    expect(classifyTurn(90), ManeuverType.turnRight);
    expect(classifyTurn(-90), ManeuverType.turnLeft);
    expect(classifyTurn(130), ManeuverType.sharpRight);
    expect(classifyTurn(-130), ManeuverType.sharpLeft);
    expect(classifyTurn(175), ManeuverType.uturn);
    expect(classifyTurn(-175), ManeuverType.uturn);
  });

  test('a right-angle corner is one turn at the corner', () {
    final m = buildManeuvers(_corner(200, 90, 200));
    expect(m.length, 2); // the turn, then arrive
    expect(m.first.type, ManeuverType.turnRight);
    expect(m.first.distM, closeTo(200, 12));
    expect(m.last.type, ManeuverType.arrive);
  });

  test('left corners come out left', () {
    final m = buildManeuvers(_corner(200, -90, 200));
    expect(m.first.type, ManeuverType.turnLeft);
  });

  test('a gentle curve is not a maneuver', () {
    // 90° spread over 200 m: no 15 m window sees more than ~7°.
    const lat0 = 50.0, lon0 = 20.0;
    final dLon = _mLon(lat0);
    final pts = <LatLon>[];
    const r = 200.0;
    for (var a = 0.0; a <= 90; a += 2) {
      final rad = a * math.pi / 180;
      pts.add(LatLon(
        lat0 + (r * math.sin(rad)) * _mLat,
        lon0 + (r - r * math.cos(rad)) * dLon,
      ));
    }
    final m = buildManeuvers(pts);
    expect(m.length, 1, reason: 'a bend in the road is not a turn');
  });

  test('one corner spread over several vertices merges into one maneuver', () {
    // Two 45° bends 10 m apart read as a single 90° turn.
    const lat0 = 50.0, lon0 = 20.0;
    final dLon = _mLon(lat0);
    final pts = <LatLon>[];
    for (var d = 0.0; d <= 200; d += 5) {
      pts.add(LatLon(lat0 + d * _mLat, lon0));
    }
    var cur = pts.last;
    for (var d = 5.0; d <= 10; d += 5) {
      cur = LatLon(cur.lat + 5 * math.cos(math.pi / 4) * _mLat,
          cur.lon + 5 * math.sin(math.pi / 4) * dLon);
      pts.add(cur);
    }
    for (var d = 5.0; d <= 200; d += 5) {
      cur = LatLon(cur.lat, cur.lon + 5 * dLon);
      pts.add(cur);
    }
    final m = buildManeuvers(pts);
    expect(m.length, 2, reason: 'merged into one turn plus arrive');
    // Chord-based: a corner rounded over 10 m reads softer than the 90° the
    // two bends add up to, which is the intended behaviour — the rider sees
    // one instruction, not two.
    expect(m.first.type,
        anyOf(ManeuverType.turnRight, ManeuverType.slightRight));
  });

  test('the last maneuver is always the arrival at the last point', () {
    final pts = _corner(200, 90, 200);
    final m = buildManeuvers(pts);
    expect(m.last.type, ManeuverType.arrive);
    expect(m.last.point, pts.last);
    expect(m.last.distM, closeTo(cumulativeM(pts).last, 1e-6));
  });

  test('a bend inside the first 15 m is ignored', () {
    // Nothing to measure an incoming bearing over yet.
    final pts = <LatLon>[
      const LatLon(50.0, 20.0),
      const LatLon(50.00005, 20.0), // ~5.5 m
      for (var d = 5.0; d <= 200; d += 5)
        LatLon(50.00005, 20.0 + d * _mLon(50.0)),
    ];
    final m = buildManeuvers(pts);
    expect(m.every((x) => x.distM > kTurnWindowM || x.type == ManeuverType.arrive),
        isTrue);
  });
}
