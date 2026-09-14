/// The next-turn plate at the top of the map, and the route summary below it.
library;

import 'package:flutter/material.dart';

import '../../i18n/strings.dart';
import '../../nav/maneuvers.dart';
import '../../nav/route_guide.dart';

const Map<ManeuverType, IconData> kManeuverIcons = <ManeuverType, IconData>{
  ManeuverType.straight: Icons.straight,
  ManeuverType.slightLeft: Icons.turn_slight_left,
  ManeuverType.slightRight: Icons.turn_slight_right,
  ManeuverType.turnLeft: Icons.turn_left,
  ManeuverType.turnRight: Icons.turn_right,
  ManeuverType.sharpLeft: Icons.turn_sharp_left,
  ManeuverType.sharpRight: Icons.turn_sharp_right,
  ManeuverType.uturn: Icons.u_turn_left,
  ManeuverType.arrive: Icons.sports_score,
};

/// Metres below a kilometre, kilometres above — the precision a rider can act on.
String formatDistance(BuildContext context, double metres) {
  if (metres < 1000) {
    final rounded = metres < 100 ? (metres / 10).round() * 10 : metres.round();
    return tf(context, 'nav.dist.m', <String, Object?>{'n': rounded});
  }
  return tf(context, 'nav.dist.km',
      <String, Object?>{'n': (metres / 1000).toStringAsFixed(1)});
}

class ManeuverBanner extends StatelessWidget {
  const ManeuverBanner({super.key, required this.guidance});

  final Guidance guidance;

  @override
  Widget build(BuildContext context) {
    final scheme = Theme.of(context).colorScheme;
    if (guidance.offRoute) {
      return _Plate(
        background: scheme.error,
        foreground: scheme.onError,
        icon: Icons.warning_amber,
        text: t(context, 'nav.guide.offRoute'),
      );
    }
    final type = guidance.next.type;
    return _Plate(
      background: Colors.black.withValues(alpha: 0.78),
      foreground: Colors.white,
      icon: kManeuverIcons[type] ?? Icons.straight,
      text: type == ManeuverType.arrive && guidance.arrived
          ? t(context, 'nav.guide.arrived')
          : formatDistance(context, guidance.nextDistM),
      semantics: t(context, type.i18nKey),
    );
  }
}

class _Plate extends StatelessWidget {
  const _Plate({
    required this.background,
    required this.foreground,
    required this.icon,
    required this.text,
    this.semantics,
  });

  final Color background;
  final Color foreground;
  final IconData icon;
  final String text;
  final String? semantics;

  @override
  Widget build(BuildContext context) {
    return Semantics(
      label: semantics == null ? text : '$semantics, $text',
      child: Container(
        padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 10),
        decoration: BoxDecoration(
          color: background,
          borderRadius: BorderRadius.circular(12),
        ),
        child: Row(
          mainAxisSize: MainAxisSize.min,
          children: <Widget>[
            Icon(icon, color: foreground, size: 28),
            const SizedBox(width: 10),
            Text(text,
                style: TextStyle(
                    color: foreground,
                    fontSize: 20,
                    fontWeight: FontWeight.w600)),
          ],
        ),
      ),
    );
  }
}
