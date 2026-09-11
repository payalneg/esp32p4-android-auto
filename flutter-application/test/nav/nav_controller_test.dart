/// The tap flow is where a route is actually made, and its states are easy to
/// get subtly wrong — asking for a point that is already known, or forgetting
/// one that is.
library;

import 'dart:async';

import 'package:aa_bridge/nav/announcer.dart';
import 'package:aa_bridge/nav/geo.dart';
import 'package:aa_bridge/nav/location_service.dart';
import 'package:aa_bridge/nav/map_data.dart';
import 'package:aa_bridge/nav/nav_controller.dart';
import 'package:aa_bridge/nav/rgf2.dart';
import 'package:aa_bridge/nav/way_classes.dart';
import 'package:flutter_test/flutter_test.dart';

import 'rgf2_packer.dart';

/// Two parallel streets 107 m apart, joined at both ends:
///
///   A ─ B ─ C ─ D      lon 20.0000 (the route)
///   │           │
///   E ───────── F      lon 20.0015 (where the rider actually goes)
RouteGraph _twoStreets() {
  final res = kWayClasses.indexOf('residential');
  TestEdge e(int from, int to, int lenDm, List<(double, double)> pts) =>
      TestEdge(from: from, to: to, lenDm: lenDm, wayClass: res, points: pts);
  return RouteGraph.parse(packGraph(
    <(double, double)>[
      (50.000, 20.0000), // A
      (50.001, 20.0000), // B
      (50.002, 20.0000), // C
      (50.003, 20.0000), // D
      (50.000, 20.0015), // E
      (50.003, 20.0015), // F
    ],
    <TestEdge>[
      e(0, 1, 1112, const [(50.000, 20.0), (50.001, 20.0)]),
      e(1, 2, 1112, const [(50.001, 20.0), (50.002, 20.0)]),
      e(2, 3, 1112, const [(50.002, 20.0), (50.003, 20.0)]),
      e(0, 4, 1073, const [(50.000, 20.0), (50.000, 20.0015)]),
      e(4, 5, 3336, const [(50.000, 20.0015), (50.003, 20.0015)]),
      e(5, 3, 1073, const [(50.003, 20.0015), (50.003, 20.0)]),
    ],
  ));
}

/// Lets the controller's async route calls run to completion.
Future<void> settle() async {
  for (var i = 0; i < 5; i++) {
    await Future<void>.delayed(Duration.zero);
  }
}

GeoFix _fix(double lat, double lon) => GeoFix(
    position: LatLon(lat, lon), speedMs: 5, headingDeg: 0, accuracyM: 8);

void main() {
  const a = LatLon(50.06, 19.94);
  const b = LatLon(49.98, 20.06);

  test('two taps set the ends and ask for nothing more', () async {
    final c = NavController()..toggleTapMode();
    expect(c.tapMode, TapMode.start);

    c.onMapTap(a);
    expect(c.start, a);
    expect(c.tapMode, TapMode.finish);
    expect(c.messageKey, 'nav.route.tapFinish');

    c.onMapTap(b);
    expect(c.finish, b);
    expect(c.tapMode, TapMode.none);
  });

  test('a destination chosen from search only needs a start', () async {
    // The bug this pins: after setFinish the controller asked for a
    // destination again once the start arrived, though it already had one.
    final c = NavController();
    await c.setFinish(b);
    expect(c.finish, b);
    expect(c.tapMode, TapMode.start, reason: 'the start is what is missing');

    c.onMapTap(a);
    expect(c.start, a);
    expect(c.finish, b, reason: 'the destination survives');
    expect(c.tapMode, TapMode.none, reason: 'and is not asked for twice');
  });

  test('a start chosen from search only needs a destination', () async {
    final c = NavController();
    await c.setStart(a);
    expect(c.tapMode, TapMode.finish);
    c.onMapTap(b);
    expect(c.start, a);
    expect(c.finish, b);
  });

  test('taps do nothing until the mode is on', () {
    final c = NavController();
    c.onMapTap(a);
    expect(c.start, isNull);
    expect(c.finish, isNull);
  });

  test('reset clears the pair and the prompt', () {
    final c = NavController()..toggleTapMode();
    c.onMapTap(a);
    c.reset();
    expect(c.start, isNull);
    expect(c.tapMode, TapMode.none);
    expect(c.messageKey, isNull);
  });

  group('navigation', () {
    late NavController c;
    late StreamController<GeoFix> gps;
    late List<Announcement> said;

    setUp(() async {
      c = NavController(mapData: MapData.forTest(_twoStreets()));
      gps = StreamController<GeoFix>();
      said = <Announcement>[];
      c.announcements.listen(said.add);
      await c.setStart(const LatLon(50.000, 20.0));
      await c.setFinish(const LatLon(50.003, 20.0));
      await settle();
      expect(c.route, isNotNull, reason: 'A→D straight up the west street');
      expect(c.route!.lengthM, closeTo(333.6, 2));
    });

    tearDown(() {
      // Not awaited: close() on a controller nobody listened to completes
      // only once someone does, which here is never.
      unawaited(gps.close());
      c.dispose();
    });

    test('cannot start without a route, can with one', () {
      final bare = NavController(mapData: MapData.forTest(_twoStreets()));
      expect(bare.startNavigation(), isFalse);
      expect(bare.messageKey, 'nav.sim.needRoute');
      bare.dispose();

      expect(c.startNavigation(), isTrue);
      expect(c.navigating, isTrue);
      expect(c.follow, isTrue, reason: 'guidance without following is a map');
      c.stopNavigation();
      expect(c.navigating, isFalse);
      expect(c.route, isNotNull, reason: 'the route outlives the ride');
    });

    test('leaving the route brings a new one from where you are', () async {
      c.attachFixes(gps.stream);
      c.startNavigation();
      final before = c.route;

      // Riding up the EAST street: 107 m off the line, well past 30 m.
      gps.add(_fix(50.0005, 20.0015));
      await settle();
      gps.add(_fix(50.0010, 20.0015));
      await settle();
      expect(said, isEmpty, reason: 'two fixes are hysteresis, not a verdict');

      gps.add(_fix(50.0015, 20.0015));
      await settle();
      expect(said.map((x) => x.kind), <AnnouncementKind>[AnnouncementKind.rerouting]);
      expect(c.start, const LatLon(50.0015, 20.0015),
          reason: 'the new route starts under the rider');
      expect(identical(c.route, before), isFalse);
      expect(c.route, isNotNull);
      expect(c.finish, const LatLon(50.003, 20.0), reason: 'same destination');
      expect(c.navigating, isTrue);

      // Still off whatever the router snapped to: no second reroute inside
      // the cooldown, however many fixes say so.
      gps.add(_fix(50.0020, 20.0015));
      await settle();
      gps.add(_fix(50.0022, 20.0015));
      await settle();
      gps.add(_fix(50.0024, 20.0015));
      await settle();
      expect(said.where((x) => x.kind == AnnouncementKind.rerouting), hasLength(1));
    });

    test('a poor fix never triggers a reroute', () async {
      c.attachFixes(gps.stream);
      c.startNavigation();
      for (var i = 0; i < 5; i++) {
        gps.add(GeoFix(
            position: LatLon(50.0005 + i * 0.0003, 20.0015),
            speedMs: 5,
            accuracyM: 120)); // a tunnel, a tall street
        await settle();
      }
      expect(said, isEmpty);
      expect(c.guidance!.offRoute, isTrue, reason: 'the banner still shows it');
    });

    test('nothing is said, and nothing rerouted, when only following', () async {
      c.attachFixes(gps.stream);
      c.setFollow(true); // no startNavigation
      for (var i = 0; i < 4; i++) {
        gps.add(_fix(50.0005 + i * 0.0005, 20.0015));
        await settle();
      }
      expect(said, isEmpty);
      expect(c.start, const LatLon(50.000, 20.0), reason: 'route untouched');
    });

    test('arriving ends the navigation', () async {
      c.attachFixes(gps.stream);
      c.startNavigation();
      gps.add(_fix(50.0029, 20.0)); // 11 m short of D
      await settle();
      expect(c.guidance!.arrived, isTrue);
      expect(c.navigating, isFalse);
      expect(c.messageKey, 'nav.guide.arrived');
      expect(said.last.kind, AnnouncementKind.arrived);
    });
  });
}
