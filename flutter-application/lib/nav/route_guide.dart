/// Guidance along a built route: where we are on it, what is next, and whether
/// we have left it. Port of `Navigator` in scripts/mapgen/navigator.py —
/// renamed because `Navigator` is Flutter's own.
library;

import 'dart:typed_data';

import 'geo.dart';
import 'maneuvers.dart';
import 'router.dart';

const double kOffRouteM = 30.0; // further than this counts as leaving
const double kBackOnM = 15.0; // closer than this counts as returning
const int kOffRouteHits = 3; // consecutive fixes before we say so
const double kSearchAheadM = 300.0;
const double kSearchBackM = 60.0;
const double kArriveM = 15.0;
const double kBikeSpeedMs = 18 / 3.6;

/// A snapshot of guidance for one position fix.
class Guidance {
  const Guidance({
    required this.alongM,
    required this.remainingM,
    required this.remainingS,
    required this.offsetM,
    required this.offRoute,
    required this.arrived,
    required this.next,
    required this.nextDistM,
  });

  final double alongM;
  final double remainingM;
  final double remainingS;

  /// Perpendicular distance from the route line.
  final double offsetM;
  final bool offRoute;
  final bool arrived;
  final Maneuver next;

  /// Distance from here to [next].
  final double nextDistM;
}

class RouteGuide {
  RouteGuide(RouteResult route)
      : points = route.points,
        cum = cumulativeM(route.points),
        maneuvers = buildManeuvers(route.points) {
    totalM = cum.isEmpty ? 0 : cum[cum.length - 1];
  }

  final List<LatLon> points;
  final Float64List cum;
  final List<Maneuver> maneuvers;
  late final double totalM;

  double alongM = 0;
  double offsetM = 0;
  bool offRoute = false;
  int _offHits = 0;

  /// Where on the route [pos] sits.
  ///
  /// The search is windowed around the previous match rather than run over the
  /// whole line: on a route that loops or doubles back, a global nearest-point
  /// search jumps to the wrong leg. A global pass is still used as a fallback
  /// when nothing close turns up in the window — a re-acquired GPS fix, or a
  /// start from the middle of the route, would otherwise stick to the window
  /// edge forever.
  ({double along, double offset}) _match(LatLon pos) {
    final loM = alongM - kSearchBackM;
    final hiM = alongM + kSearchAheadM;
    double? bestAlong;
    var bestOffset = double.infinity;
    for (var i = 0; i < points.length - 1; i++) {
      if (cum[i + 1] < loM || cum[i] > hiM) continue;
      final p = projectToSegment(pos, points[i], points[i + 1]);
      if (p.offsetM < bestOffset) {
        bestOffset = p.offsetM;
        bestAlong = cum[i] + p.t * (cum[i + 1] - cum[i]);
      }
    }
    if (bestAlong == null || bestOffset > kOffRouteM) {
      for (var i = 0; i < points.length - 1; i++) {
        final p = projectToSegment(pos, points[i], points[i + 1]);
        if (p.offsetM < bestOffset) {
          bestOffset = p.offsetM;
          bestAlong = cum[i] + p.t * (cum[i + 1] - cum[i]);
        }
      }
    }
    return (along: bestAlong ?? 0, offset: bestOffset);
  }

  Guidance update(LatLon pos) {
    final m = _match(pos);
    alongM = m.along;
    offsetM = m.offset;

    // Hysteresis: one bad fix should not flip the banner, and coming back
    // needs to be convincing before it flips again.
    if (offsetM > kOffRouteM) {
      _offHits++;
      if (_offHits >= kOffRouteHits) offRoute = true;
    } else if (offsetM < kBackOnM) {
      _offHits = 0;
      offRoute = false;
    }

    var next = maneuvers.last;
    for (final man in maneuvers) {
      if (man.distM >= alongM - 1) {
        next = man;
        break;
      }
    }
    final remaining = (totalM - alongM).clamp(0.0, double.infinity);
    return Guidance(
      alongM: alongM,
      remainingM: remaining,
      remainingS: remaining / kBikeSpeedMs,
      offsetM: offsetM,
      offRoute: offRoute,
      arrived: remaining < kArriveM,
      next: next,
      nextDistM: (next.distM - alongM).clamp(0.0, double.infinity),
    );
  }

  /// Point [alongM] metres into the route — used to drive the ride simulator.
  LatLon positionAt(double along) => pointAtDistance(points, cum, along);
}
