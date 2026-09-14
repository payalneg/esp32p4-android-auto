/// Drives a virtual rider along a route.
///
/// Emits the same [GeoFix] the GPS does, so guidance, the map marker and the
/// maneuver banner exercise the real code path — the simulation is a test
/// harness for the navigator, not a separate mode of it.
library;

import 'dart:async';

import 'geo.dart';
import 'location_service.dart';
import 'route_guide.dart';

/// Faster than a real ride so a 8 km route can be watched end to end.
const double kSimSpeedup = 3.0;
const Duration kSimTick = Duration(milliseconds: 200);

Stream<GeoFix> simulateRide(
  RouteGuide guide, {
  double speedMs = kBikeSpeedMs,
  double speedup = kSimSpeedup,
  Duration tick = kSimTick,
}) {
  var along = 0.0;
  late StreamController<GeoFix> controller;
  Timer? timer;

  void stop() {
    timer?.cancel();
    timer = null;
  }

  void start() {
    timer = Timer.periodic(tick, (_) {
      final step = speedMs * speedup * tick.inMilliseconds / 1000.0;
      final previous = guide.positionAt(along);
      along += step;
      final position = guide.positionAt(along);
      controller.add(GeoFix(
        position: position,
        speedMs: speedMs * speedup,
        headingDeg: bearingDeg(previous, position),
      ));
      if (along >= guide.totalM) {
        stop();
        controller.close();
      }
    });
  }

  controller = StreamController<GeoFix>(
    onListen: start,
    onCancel: stop,
    onPause: stop,
    onResume: start,
  );
  return controller.stream;
}
