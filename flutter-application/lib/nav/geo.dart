/// Geodesy helpers for the offline navigator.
///
/// Deliberately dependency-free (no latlong2, no flutter_map): everything in
/// `lib/nav` is plain Dart so it can be unit-tested without a Flutter binding
/// and, later, re-read against the C port on the head unit. The UI converts
/// [LatLon] to the map widget's `LatLng` at its own boundary.
///
/// Ported 1:1 from scripts/mapgen/navigator.py — the constants and the maths
/// must agree, because both sides are checked against the same routes.
library;

import 'dart:math' as math;
import 'dart:typed_data';

const double kEarthRadiusM = 6371000.0;

/// A WGS84 position. Value type: two of these are equal when both fields are.
class LatLon {
  const LatLon(this.lat, this.lon);

  final double lat;
  final double lon;

  @override
  bool operator ==(Object other) =>
      other is LatLon && other.lat == lat && other.lon == lon;

  @override
  int get hashCode => Object.hash(lat, lon);

  @override
  String toString() => '(${lat.toStringAsFixed(6)}, ${lon.toStringAsFixed(6)})';
}

/// Great-circle distance in metres.
double haversineM(LatLon a, LatLon b) {
  final p1 = a.lat * math.pi / 180.0;
  final p2 = b.lat * math.pi / 180.0;
  final dp = p2 - p1;
  final dl = (b.lon - a.lon) * math.pi / 180.0;
  final s1 = math.sin(dp / 2);
  final s2 = math.sin(dl / 2);
  final h = s1 * s1 + math.cos(p1) * math.cos(p2) * s2 * s2;
  return 2 * kEarthRadiusM * math.asin(math.sqrt(h.clamp(0.0, 1.0)));
}

/// Initial bearing from [a] to [b], degrees clockwise from north, in [0, 360).
double bearingDeg(LatLon a, LatLon b) {
  final p1 = a.lat * math.pi / 180.0;
  final p2 = b.lat * math.pi / 180.0;
  final dl = (b.lon - a.lon) * math.pi / 180.0;
  final y = math.sin(dl) * math.cos(p2);
  final x = math.cos(p1) * math.sin(p2) -
      math.sin(p1) * math.cos(p2) * math.cos(dl);
  final deg = math.atan2(y, x) * 180.0 / math.pi;
  return deg % 360.0;
}

/// Signed turn from one bearing to another: >0 right, <0 left, in (-180, 180].
double turnDelta(double bearingIn, double bearingOut) =>
    (bearingOut - bearingIn + 180.0) % 360.0 - 180.0;

/// Distance from the route start to every vertex, metres. Length == points.
Float64List cumulativeM(List<LatLon> points) {
  final out = Float64List(points.length);
  for (var i = 1; i < points.length; i++) {
    out[i] = out[i - 1] + haversineM(points[i - 1], points[i]);
  }
  return out;
}

/// The point [m] metres along the polyline, interpolated inside the segment.
/// Clamps to the ends. [cum] must come from [cumulativeM] for [points].
LatLon pointAtDistance(List<LatLon> points, Float64List cum, double m) {
  if (points.isEmpty) throw ArgumentError('empty polyline');
  if (m <= 0) return points.first;
  if (m >= cum[cum.length - 1]) return points.last;
  var lo = 0;
  var hi = cum.length - 1;
  while (hi - lo > 1) {
    final mid = (lo + hi) >> 1;
    if (cum[mid] <= m) {
      lo = mid;
    } else {
      hi = mid;
    }
  }
  final seg = cum[hi] - cum[lo];
  final t = seg <= 0 ? 0.0 : (m - cum[lo]) / seg;
  final a = points[lo];
  final b = points[hi];
  return LatLon(a.lat + (b.lat - a.lat) * t, a.lon + (b.lon - a.lon) * t);
}

/// Projection of [p] onto segment [a]-[b].
///
/// Returns how far along the segment the foot of the perpendicular sits
/// (`t` in [0, 1]) and how far [p] is from it in metres. Works in a local
/// equirectangular frame — good to a fraction of a percent over the tens of
/// metres this is used for, and far cheaper than a proper geodesic.
({double t, double offsetM}) projectToSegment(LatLon p, LatLon a, LatLon b) {
  final k = math.cos(p.lat * math.pi / 180.0); // longitude squeeze
  final ax = a.lon * k, ay = a.lat;
  final bx = b.lon * k, by = b.lat;
  final px = p.lon * k, py = p.lat;
  final dx = bx - ax, dy = by - ay;
  final seg2 = dx * dx + dy * dy;
  final t = seg2 == 0
      ? 0.0
      : (((px - ax) * dx + (py - ay) * dy) / seg2).clamp(0.0, 1.0);
  final cx = ax + dx * t, cy = ay + dy * t;
  final degrees = math.sqrt((px - cx) * (px - cx) + (py - cy) * (py - cy));
  return (t: t, offsetM: degrees * 111320.0);
}
