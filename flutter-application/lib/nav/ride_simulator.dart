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
  /// Monotonic elapsed time; injectable for delayed-timer regression tests.
  Duration Function()? elapsed,
}) {
  var along = 0.0;
  late StreamController<GeoFix> controller;
  Timer? timer;
  final clock = Stopwatch();
  Duration previousTick = Duration.zero;
  var began = false;
  Duration now() => elapsed?.call() ?? clock.elapsed;

  void stop() {
    timer?.cancel();
    timer = null;
    clock.stop();
  }

  void start() {
    clock.start();
    previousTick = now();
    if (!began) {
      began = true;
      controller.add(GeoFix(
        position: guide.positionAt(0),
        speedMs: speedMs * speedup,
        headingDeg: bearingDeg(guide.positionAt(0), guide.positionAt(1)),
      ));
    }
    timer = Timer.periodic(tick, (_) {
      final current = now();
      final delta = current - previousTick;
      previousTick = current;
      if (delta <= Duration.zero) return;
      final step = speedMs * speedup * delta.inMicroseconds / 1000000.0;
      final previous = guide.positionAt(along);
      along += step;
      final position = guide.positionAt(along);
      final arrived = along >= guide.totalM;
      controller.add(GeoFix(
        position: position,
        speedMs: arrived ? 0 : speedMs * speedup,
        headingDeg: bearingDeg(previous, position),
      ));
      if (arrived) {
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
