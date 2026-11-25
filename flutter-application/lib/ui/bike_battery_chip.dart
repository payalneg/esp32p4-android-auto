/// The bike's pack charge in the app bar, with the details a rider wants when
/// they get home: percent, pack voltage, remaining Ah and how long ago the
/// reading was taken.
///
/// The number comes from the head unit (which is the only side that talks to
/// the VESC/BMS), but [BleProxy] also restores it from the phone's cache, so it
/// is visible with the bike switched off.
library;

import 'package:flutter/material.dart';

import '../ble/bike_battery.dart';
import '../ble/ble_proxy.dart';
import '../i18n/strings.dart';

class BikeBatteryChip extends StatelessWidget {
  const BikeBatteryChip({super.key});

  @override
  Widget build(BuildContext context) {
    return StreamBuilder<BikeBattery>(
      stream: BleProxy.instance.bikeBatteryUpdates,
      initialData: BleProxy.instance.bikeBattery,
      builder: (ctx, snap) {
        final battery = snap.data;
        final percent = battery?.percent;
        if (battery == null || percent == null) return const SizedBox.shrink();
        final scheme = Theme.of(ctx).colorScheme;
        final live = battery.live;
        var color = live ? scheme.onSurface : scheme.outline;
        if (percent <= 15) color = scheme.error;
        return TextButton(
          onPressed: () => _showDetails(ctx, battery),
          style: TextButton.styleFrom(
            foregroundColor: color,
            padding: const EdgeInsets.symmetric(horizontal: 8),
            minimumSize: const Size(0, 40),
            tapTargetSize: MaterialTapTargetSize.shrinkWrap,
          ),
          child: Row(
            mainAxisSize: MainAxisSize.min,
            children: [
              Icon(_icon(percent), size: 20, color: color),
              const SizedBox(width: 4),
              Text('$percent%', style: TextStyle(color: color)),
              // Grey already hints "not live"; the clock says why.
              if (!live) ...[
                const SizedBox(width: 4),
                Icon(Icons.schedule, size: 12, color: color),
              ],
            ],
          ),
        );
      },
    );
  }

  static IconData _icon(int percent) {
    if (percent >= 90) return Icons.battery_full;
    if (percent >= 70) return Icons.battery_6_bar;
    if (percent >= 50) return Icons.battery_4_bar;
    if (percent >= 30) return Icons.battery_2_bar;
    if (percent >= 10) return Icons.battery_1_bar;
    return Icons.battery_alert;
  }

  static String _when(BuildContext context, DateTime at) {
    final age = DateTime.now().difference(at);
    if (age.inMinutes < 1) return t(context, 'bikebatt.justNow');
    if (age.inMinutes < 60) {
      return tf(context, 'bikebatt.minutes', {'n': age.inMinutes});
    }
    if (age.inHours < 24) {
      return tf(context, 'bikebatt.hours', {'n': age.inHours});
    }
    return tf(context, 'bikebatt.days', {'n': age.inDays});
  }

  static void _showDetails(BuildContext context, BikeBattery battery) {
    showModalBottomSheet<void>(
      context: context,
      showDragHandle: true,
      builder: (ctx) => _BikeBatterySheet(battery: battery),
    );
  }
}

class _BikeBatterySheet extends StatelessWidget {
  const _BikeBatterySheet({required this.battery});
  final BikeBattery battery;

  @override
  Widget build(BuildContext context) {
    final scheme = Theme.of(context).colorScheme;
    final percent = battery.percent;
    final status =
        battery.live
            ? t(context, 'bikebatt.live')
            : tf(context, 'bikebatt.lastRead', {
              'when': BikeBatteryChip._when(context, battery.at),
            });
    return SafeArea(
      child: Padding(
        padding: const EdgeInsets.fromLTRB(20, 0, 20, 20),
        child: Column(
          mainAxisSize: MainAxisSize.min,
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text(
              t(context, 'bikebatt.title'),
              style: Theme.of(context).textTheme.titleMedium,
            ),
            const SizedBox(height: 12),
            Row(
              crossAxisAlignment: CrossAxisAlignment.baseline,
              textBaseline: TextBaseline.alphabetic,
              children: [
                Text(
                  percent == null ? '—' : '$percent',
                  style: const TextStyle(
                    fontSize: 48,
                    fontWeight: FontWeight.w600,
                    height: 1,
                  ),
                ),
                if (percent != null)
                  Text(
                    '%',
                    style: TextStyle(fontSize: 20, color: scheme.outline),
                  ),
                const Spacer(),
                Icon(
                  BikeBatteryChip._icon(percent ?? 0),
                  size: 36,
                  color: battery.live ? scheme.primary : scheme.outline,
                ),
              ],
            ),
            const SizedBox(height: 6),
            Text(
              status,
              style: TextStyle(
                fontSize: 12,
                color: battery.live ? scheme.primary : scheme.outline,
              ),
            ),
            if (battery.voltageV != null || battery.remainingAh != null) ...[
              const SizedBox(height: 16),
              Row(
                children: [
                  if (battery.voltageV != null)
                    Expanded(
                      child: _Detail(
                        label: t(context, 'bikebatt.voltage'),
                        value: '${battery.voltageV!.toStringAsFixed(1)} V',
                      ),
                    ),
                  if (battery.voltageV != null && battery.remainingAh != null)
                    const SizedBox(width: 12),
                  if (battery.remainingAh != null)
                    Expanded(
                      child: _Detail(
                        label: t(context, 'bikebatt.remaining'),
                        value: '${battery.remainingAh!.toStringAsFixed(1)} Ah',
                      ),
                    ),
                ],
              ),
            ],
            if (!battery.live) ...[
              const SizedBox(height: 16),
              Text(
                t(context, 'bikebatt.staleHint'),
                style: TextStyle(fontSize: 12, color: scheme.outline),
              ),
            ],
          ],
        ),
      ),
    );
  }
}

class _Detail extends StatelessWidget {
  const _Detail({required this.label, required this.value});
  final String label;
  final String value;

  @override
  Widget build(BuildContext context) {
    final scheme = Theme.of(context).colorScheme;
    return Container(
      padding: const EdgeInsets.symmetric(horizontal: 12, vertical: 10),
      decoration: BoxDecoration(
        color: scheme.surfaceContainerHighest,
        borderRadius: BorderRadius.circular(12),
      ),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Text(label, style: TextStyle(fontSize: 11, color: scheme.outline)),
          const SizedBox(height: 2),
          Text(
            value,
            style: const TextStyle(fontSize: 18, fontWeight: FontWeight.w600),
          ),
        ],
      ),
    );
  }
}
