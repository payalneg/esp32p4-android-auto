/// The only file that talks to geolocator.
///
/// Keeping the plugin behind [GeoFix] means the simulator and the real GPS
/// feed the guidance through exactly the same path, and the rest of `lib/nav`
/// stays plain Dart that unit-tests without a platform channel.
///
/// Foreground only for now: the app declares no background-location
/// permission, so guidance runs with the screen on. Adding it later means
/// ACCESS_BACKGROUND_LOCATION + FOREGROUND_SERVICE_LOCATION and moving the
/// stream into the existing foreground-service isolate.
library;

import 'package:geolocator/geolocator.dart';

import 'geo.dart';

/// One position fix, from the GPS or the ride simulator.
class GeoFix {
  const GeoFix({
    required this.position,
    this.speedMs,
    this.headingDeg,
    this.accuracyM,
  });

  final LatLon position;
  final double? speedMs;
  final double? headingDeg;
  final double? accuracyM;
}

enum LocationStatus { ok, serviceOff, denied, deniedForever }

/// A cached position older than this is somewhere you were, not where you
/// are; it is not shown.
const Duration kLastKnownMaxAge = Duration(minutes: 10);

class LocationService {
  /// Asks for what it needs, once. Returns why it cannot proceed, so the UI
  /// can offer the right remedy — the system location toggle and the app's
  /// permission page are different screens.
  Future<LocationStatus> ensurePermission() async {
    if (!await Geolocator.isLocationServiceEnabled()) {
      return LocationStatus.serviceOff;
    }
    var permission = await Geolocator.checkPermission();
    if (permission == LocationPermission.denied) {
      permission = await Geolocator.requestPermission();
    }
    switch (permission) {
      case LocationPermission.always:
      case LocationPermission.whileInUse:
        return LocationStatus.ok;
      case LocationPermission.deniedForever:
        return LocationStatus.deniedForever;
      case LocationPermission.denied:
      case LocationPermission.unableToDetermine:
        return LocationStatus.denied;
    }
  }

  /// Positions, starting with whatever the platform already knows.
  ///
  /// The first live fix can take a while — indoors, for ever — and until it
  /// came the map had no dot and the rider was told to wait. A recent cached
  /// position is good enough to draw, to route from and to start guiding;
  /// the live feed corrects it as soon as it has something better.
  Stream<GeoFix> fixes() async* {
    try {
      final last = await Geolocator.getLastKnownPosition();
      if (last != null &&
          DateTime.now().difference(last.timestamp) < kLastKnownMaxAge) {
        yield _toFix(last);
      }
    } on Object {
      // Nothing cached yet — the normal first run; the live feed follows.
    }
    yield* Geolocator.getPositionStream(
      locationSettings: const LocationSettings(
        accuracy: LocationAccuracy.best,
        distanceFilter: 2, // metres; a stationary phone stops emitting
      ),
    ).map(_toFix);
  }

  static GeoFix _toFix(Position p) => GeoFix(
        position: LatLon(p.latitude, p.longitude),
        speedMs: p.speed,
        headingDeg: p.heading,
        accuracyM: p.accuracy,
      );

  Future<void> openSystemSettings(LocationStatus status) async {
    if (status == LocationStatus.serviceOff) {
      await Geolocator.openLocationSettings();
    } else {
      await Geolocator.openAppSettings();
    }
  }
}
