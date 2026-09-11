/// Turn-by-turn maneuvers derived from route geometry.
///
/// The graph carries no street names (builder.py drops them), so a turn is
/// recognised purely from shape: at every vertex, compare the bearing coming
/// in over the last [kTurnWindowM] with the bearing going out over the next
/// [kTurnWindowM]. The window is short on purpose — a street that merely bends
/// spreads its deflection over tens of metres and never crosses the threshold,
/// while a real corner fits the whole turn inside those 15 m.
///
/// Port of `maneuvers()` in scripts/mapgen/navigator.py; thresholds must stay
/// in step with it.
library;

import 'geo.dart';

const double kTurnWindowM = 15.0;
const double kMinTurnDeg = 30.0;
const double kMergeM = 25.0;

enum ManeuverType {
  straight,
  slightLeft,
  slightRight,
  turnLeft,
  turnRight,
  sharpLeft,
  sharpRight,
  uturn,
  arrive;

  String get i18nKey => 'nav.maneuver.$name';
}

class Maneuver {
  const Maneuver({
    required this.distM,
    required this.type,
    required this.angleDeg,
    required this.point,
  });

  /// Distance from the start of the route to this maneuver.
  final double distM;
  final ManeuverType type;

  /// Signed turn angle: positive right, negative left.
  final double angleDeg;
  final LatLon point;
}

/// Classifies a signed deflection. Mirrors `_classify` in navigator.py.
ManeuverType classifyTurn(double deltaDeg) {
  final a = deltaDeg.abs();
  final right = deltaDeg > 0;
  if (a >= 160) return ManeuverType.uturn;
  if (a >= 120) return right ? ManeuverType.sharpRight : ManeuverType.sharpLeft;
  if (a >= 65) return right ? ManeuverType.turnRight : ManeuverType.turnLeft;
  if (a >= kMinTurnDeg) {
    return right ? ManeuverType.slightRight : ManeuverType.slightLeft;
  }
  return ManeuverType.straight;
}

/// Maneuver list for a route, always ending with [ManeuverType.arrive].
List<Maneuver> buildManeuvers(List<LatLon> points) {
  if (points.length < 3) {
    return <Maneuver>[
      Maneuver(
        distM: 0,
        type: ManeuverType.arrive,
        angleDeg: 0,
        point: points.isEmpty ? const LatLon(0, 0) : points.last,
      ),
    ];
  }
  final cum = cumulativeM(points);
  final total = cum[cum.length - 1];

  // Deflection at each interior vertex, measured over a fixed window so the
  // sampling density of the geometry does not change the answer.
  final candidates = <({int index, double delta})>[];
  for (var i = 1; i < points.length - 1; i++) {
    if (cum[i] < kTurnWindowM || total - cum[i] < kTurnWindowM) continue;
    final back = pointAtDistance(points, cum, cum[i] - kTurnWindowM);
    final ahead = pointAtDistance(points, cum, cum[i] + kTurnWindowM);
    final delta =
        turnDelta(bearingDeg(back, points[i]), bearingDeg(points[i], ahead));
    if (delta.abs() >= kMinTurnDeg) {
      candidates.add((index: i, delta: delta));
    }
  }

  // A single corner spans several vertices; collapse a run into one maneuver
  // and let the sharpest vertex represent it.
  final out = <Maneuver>[];
  var group = <({int index, double delta})>[];
  void flush() {
    if (group.isEmpty) return;
    var best = group.first;
    for (final c in group) {
      if (c.delta.abs() > best.delta.abs()) best = c;
    }
    out.add(Maneuver(
      distM: cum[best.index],
      type: classifyTurn(best.delta),
      angleDeg: best.delta,
      point: points[best.index],
    ));
    group = <({int index, double delta})>[];
  }

  for (final c in candidates) {
    if (group.isNotEmpty && cum[c.index] - cum[group.last.index] > kMergeM) {
      flush();
    }
    group.add(c);
  }
  flush();

  out.add(Maneuver(
    distM: total,
    type: ManeuverType.arrive,
    angleDeg: 0,
    point: points.last,
  ));
  return out;
}
