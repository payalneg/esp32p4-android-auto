/// When the navigator speaks. Fed a hand-made sequence of guidances so the
/// timing rules are checked without a route or a clock.
library;

import 'package:aa_bridge/nav/announcer.dart';
import 'package:aa_bridge/nav/geo.dart';
import 'package:aa_bridge/nav/maneuvers.dart';
import 'package:aa_bridge/nav/route_guide.dart';
import 'package:flutter_test/flutter_test.dart';

const _p = LatLon(50.0, 19.9);

Maneuver _man(ManeuverType type, double at) =>
    Maneuver(distM: at, type: type, angleDeg: 90, point: _p);

Guidance _g(Maneuver next, double distTo, {bool arrived = false}) => Guidance(
      alongM: next.distM - distTo,
      remainingM: 1000,
      remainingS: 200,
      offsetM: 0,
      offRoute: false,
      arrived: arrived,
      next: next,
      nextDistM: distTo,
    );

void main() {
  test('a turn gets one warning and one call, in that order', () {
    final a = TurnAnnouncer();
    final left = _man(ManeuverType.turnLeft, 500);
    const v = 5.0; // 18 km/h: prepare at 100 m, call at 20 m
    expect(a.update(_g(left, 300), v), isEmpty, reason: 'still far');
    final prep = a.update(_g(left, 95), v);
    expect(prep, hasLength(1));
    expect(prep.single.kind, AnnouncementKind.prepare);
    expect(prep.single.maneuver, ManeuverType.turnLeft);
    expect(prep.single.distM, 100, reason: 'rounded to 50');
    expect(a.update(_g(left, 60), v), isEmpty, reason: 'said already');
    final now = a.update(_g(left, 15), v);
    expect(now.single.kind, AnnouncementKind.now);
    expect(a.update(_g(left, 5), v), isEmpty);
  });

  test('faster riders are warned earlier', () {
    final a = TurnAnnouncer();
    final right = _man(ManeuverType.turnRight, 900);
    // 40 km/h → 11 m/s × 15 s = 167 m of warning.
    expect(a.update(_g(right, 200), 11.1), isEmpty);
    final prep = a.update(_g(right, 160), 11.1);
    expect(prep.single.kind, AnnouncementKind.prepare);
    expect(prep.single.distM, 150);
  });

  test('warning distance is clamped at both ends', () {
    final slow = TurnAnnouncer();
    final m = _man(ManeuverType.turnLeft, 900);
    // Walking: 15 s would be 18 m, but the floor is 100 m.
    expect(slow.update(_g(m, 99), 1.2).single.kind, AnnouncementKind.prepare);
    final fast = TurnAnnouncer();
    // Very fast: 15 s would be 600 m, but the ceiling is 400 m.
    expect(fast.update(_g(m, 450), 40), isEmpty);
    expect(fast.update(_g(m, 390), 40).single.kind, AnnouncementKind.prepare);
  });

  test('a junction right after the start gets only the immediate call', () {
    final a = TurnAnnouncer();
    final m = _man(ManeuverType.turnRight, 15);
    final out = a.update(_g(m, 15), 4);
    expect(out, hasLength(1), reason: 'no "in 50 metres" for 15 metres');
    expect(out.single.kind, AnnouncementKind.now);
  });

  test('each turn is announced on its own', () {
    final a = TurnAnnouncer();
    final first = _man(ManeuverType.turnLeft, 200);
    final second = _man(ManeuverType.turnRight, 260);
    expect(a.update(_g(first, 90), 4).single.kind, AnnouncementKind.prepare);
    expect(a.update(_g(first, 10), 4).single.kind, AnnouncementKind.now);
    // Past the first; the second is 60 m on — warned at once, not skipped.
    final prep = a.update(_g(second, 55), 4);
    expect(prep.single.kind, AnnouncementKind.prepare);
    expect(prep.single.maneuver, ManeuverType.turnRight);
  });

  test('straight-on forks are not spoken', () {
    final a = TurnAnnouncer();
    final s = _man(ManeuverType.straight, 300);
    expect(a.update(_g(s, 90), 4), isEmpty);
    expect(a.update(_g(s, 10), 4), isEmpty);
  });

  test('the destination is announced ahead, then once on arrival', () {
    final a = TurnAnnouncer();
    final end = _man(ManeuverType.arrive, 1000);
    final soon = a.update(_g(end, 95), 4);
    expect(soon.single.kind, AnnouncementKind.arriveSoon);
    expect(soon.single.distM, 100);
    expect(a.update(_g(end, 30), 4), isEmpty, reason: 'no "now" for arrive');
    final done = a.update(_g(end, 5, arrived: true), 0);
    expect(done.single.kind, AnnouncementKind.arrived);
    expect(a.update(_g(end, 2, arrived: true), 0), isEmpty);
  });

  test('reset forgets everything, as a new route should', () {
    final a = TurnAnnouncer();
    final m = _man(ManeuverType.turnLeft, 500);
    a.update(_g(m, 95), 4);
    a.reset();
    expect(a.update(_g(m, 95), 4), hasLength(1));
  });
}
