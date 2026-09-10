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

  Stream<GeoFix> fixes() => Geolocator.getPositionStream(
        locationSettings: const LocationSettings(
          accuracy: LocationAccuracy.best,
          distanceFilter: 2, // metres; a stationary phone stops emitting
        ),
      ).map((p) => GeoFix(
            position: LatLon(p.latitude, p.longitude),
            speedMs: p.speed,
            headingDeg: p.heading,
            accuracyM: p.accuracy,
          ));

  Future<void> openSystemSettings(LocationStatus status) async {
    if (status == LocationStatus.serviceOff) {
      await Geolocator.openLocationSettings();
    } else {
      await Geolocator.openAppSettings();
    }
  }
}
