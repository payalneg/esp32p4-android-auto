/// Where the map camera should be while navigating.
///
/// Pure arithmetic from the latest fix and guidance: zoom in when slow or
/// close to a turn, out when fast; turn the map so the direction of travel is
/// up; and put the rider in the lower part of the screen so most of it shows
/// what is coming rather than what has passed. The screen applies the result
/// with one `moveAndRotate`.
library;

import 'dart:math' as math;

import 'geo.dart';
import 'location_service.dart';
import 'route_guide.dart';

/// Below this the GPS heading is noise and the map keeps its last rotation.
const double kHeadingMinSpeedMs = 1.5;

/// Low-pass weight for the heading — high enough to follow a real turn
/// within a couple of fixes, low enough not to twitch on every fix.
const double kHeadingSmoothing = 0.4;

/// Fraction of the viewport height the rider sits above the bottom edge in
/// heading-up mode.
const double kRiderFromBottom = 0.3;

/// Zoom in this close to a turn, whatever the speed.
const double kZoomInBeforeTurnM = 150;

class CameraPose {
  const CameraPose(this.center, this.zoom, this.rotationDeg);

  final LatLon center;
  final double zoom;

  /// Map rotation in flutter_map's terms: 0 is north up.
  final double rotationDeg;
}

class NavCamera {
  double? _headingDeg;

  /// Smoothed direction of travel, or null before the first usable fix.
  double? get headingDeg => _headingDeg;

  void reset() => _headingDeg = null;

  CameraPose pose(
    GeoFix fix,
    Guidance? guidance, {
    required bool trackUp,
    required double viewportHeightPx,
  }) {
    final speed = fix.speedMs ?? 0;
    _updateHeading(fix.headingDeg, speed);

    var zoom = zoomForSpeed(speed);
    if (guidance != null && guidance.nextDistM < kZoomInBeforeTurnM) {
      zoom = math.max(zoom, 18);
    }

    final h = _headingDeg;
    if (!trackUp || h == null) {
      return CameraPose(fix.position, zoom, 0);
    }
    // Shift the centre ahead along the heading so the rider's dot lands at
    // kRiderFromBottom of the screen instead of the middle.
    final aheadPx = viewportHeightPx * (0.5 - kRiderFromBottom);
    final aheadM = aheadPx * metresPerPixel(zoom, fix.position.lat);
    return CameraPose(_destination(fix.position, h, aheadM), zoom, -h);
  }

  void _updateHeading(double? raw, double speed) {
    if (raw == null || raw.isNaN || raw < 0 || speed < kHeadingMinSpeedMs) {
      return;
    }
    final prev = _headingDeg;
    if (prev == null) {
      _headingDeg = raw % 360;
      return;
    }
    // Blend the short way round the circle, or a 350° → 10° turn swings the
    // map through south.
    var delta = (raw - prev + 540) % 360 - 180;
    _headingDeg = (prev + delta * kHeadingSmoothing + 360) % 360;
  }

  /// Slow — a walker, a junction — gets detail; fast gets a look ahead.
  static double zoomForSpeed(double speedMs) {
    if (speedMs < 3) return 18;
    if (speedMs < 7) return 17.5;
    if (speedMs < 12) return 17;
    return 16.5;
  }

  /// Ground distance of one logical pixel at [zoom] (256 px tiles).
  static double metresPerPixel(double zoom, double lat) =>
      156543.03392 * math.cos(lat * math.pi / 180) / math.pow(2, zoom);

  /// The point [distM] metres from [p] along [bearingDeg].
  static LatLon _destination(LatLon p, double bearingDeg, double distM) {
    const r = 6371008.8;
    final lat1 = p.lat * math.pi / 180;
    final lon1 = p.lon * math.pi / 180;
    final brg = bearingDeg * math.pi / 180;
    final ang = distM / r;
    final lat2 = math.asin(math.sin(lat1) * math.cos(ang) +
        math.cos(lat1) * math.sin(ang) * math.cos(brg));
    final lon2 = lon1 +
        math.atan2(math.sin(brg) * math.sin(ang) * math.cos(lat1),
            math.cos(ang) - math.sin(lat1) * math.sin(lat2));
    return LatLon(lat2 * 180 / math.pi, lon2 * 180 / math.pi);
  }
}
