/// The bar under the map: what the route costs, or what the navigator is
/// waiting for.
library;

import 'package:flutter/material.dart';

import '../../i18n/strings.dart';
import '../../nav/nav_controller.dart';

class RouteInfoBar extends StatelessWidget {
  const RouteInfoBar({
    super.key,
    required this.controller,
    required this.onSaveOffline,
  });

  final NavController controller;

  /// Downloads the tiles along the route; null while there is nothing to save.
  final VoidCallback? onSaveOffline;

  @override
  Widget build(BuildContext context) {
    final route = controller.route;
    final guidance = controller.guidance;
    final message = controller.messageKey;

    String? text;
    if (route != null) {
      // While riding, the remaining distance is the useful number; before
      // setting off, the whole trip is.
      text = guidance != null && guidance.alongM > 1
          ? tf(context, 'nav.guide.remaining', <String, Object?>{
              'km': (guidance.remainingM / 1000).toStringAsFixed(1),
              'min': (guidance.remainingS / 60).round(),
            })
          : tf(context, 'nav.route.summary', <String, Object?>{
              'km': (route.lengthM / 1000).toStringAsFixed(1),
              'min': (route.timeS / 60).round(),
              'pct': route.footSharePct.round(),
            });
    } else if (message != null) {
      text = t(context, message);
    }
    if (text == null) return const SizedBox.shrink();

    return Card(
      margin: EdgeInsets.zero,
      color: Theme.of(context).colorScheme.surfaceContainerHigh,
      child: Padding(
        padding: const EdgeInsets.fromLTRB(14, 8, 6, 8),
        child: Row(
          mainAxisSize: MainAxisSize.min,
          children: <Widget>[
            if (controller.routing)
              const Padding(
                padding: EdgeInsets.only(right: 10),
                child: SizedBox(
                    width: 16,
                    height: 16,
                    child: CircularProgressIndicator(strokeWidth: 2)),
              ),
            Flexible(child: Text(text, overflow: TextOverflow.ellipsis)),
            if (onSaveOffline != null)
              IconButton(
                icon: const Icon(Icons.download_for_offline_outlined),
                tooltip: t(context, 'mapdata.corridor'),
                onPressed: onSaveOffline,
              ),
          ],
        ),
      ),
    );
  }
}
