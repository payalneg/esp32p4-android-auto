/// Hub with the app's three jobs:
///   * Display settings — everything about the head unit (link, permissions,
///     pairing, firmware, files). See display_settings_screen.dart.
///   * BLE helper settings — the ESP32-C3 board that bridges BLE buttons and a
///     cadence sensor to the VESC over CAN. See helper_screen.dart.
///   * LISP editor — edit the VESC's script over any NUS link: the head unit's
///     bridge, a stand-alone VESC BLE adapter, or the helper (it exposes the
///     same NUS service), so it works with no head unit at all. See
///     lisp_editor_screen.dart.
library;

import 'package:flutter/material.dart';

import '../ble/ble_proxy.dart';
import '../ble/lisp_models.dart';
import '../helper/helper_proxy.dart';
import '../i18n/strings.dart';
import 'about_screen.dart';
import 'display_settings_screen.dart';
import 'helper_screen.dart';
import 'lisp_editor_screen.dart';

class HomeScreen extends StatelessWidget {
  const HomeScreen({super.key});

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: Text(t(context, 'app.title')),
        actions: [
          IconButton(
            icon: const Icon(Icons.translate),
            tooltip: t(context, 'home.lang.title'),
            onPressed: () => _showLanguagePicker(context),
          ),
        ],
      ),
      body: ListView(
        padding: const EdgeInsets.all(16),
        children: [
          StreamBuilder<BleConnState>(
            stream: BleProxy.instance.state,
            initialData: BleProxy.instance.currentState,
            builder: (ctx, snap) {
              final connected = snap.data == BleConnState.connected;
              return _HubCard(
                icon: connected ? Icons.tv : Icons.tv_off,
                iconColor: connected ? Colors.green : null,
                title: t(context, 'home.display.title'),
                subtitle: connected
                    ? t(context, 'home.status.connected')
                    : t(context, 'home.status.disconnected'),
                onTap: () => Navigator.push(
                  context,
                  MaterialPageRoute(
                      builder: (_) => const DisplaySettingsScreen()),
                ),
              );
            },
          ),
          ListenableBuilder(
            listenable: HelperProxy.instance,
            builder: (ctx, _) {
              final connected = HelperProxy.instance.connected;
              return _HubCard(
                icon: connected
                    ? Icons.settings_remote
                    : Icons.settings_remote_outlined,
                iconColor: connected ? Colors.green : null,
                title: t(context, 'home.helper.title'),
                subtitle: connected
                    ? t(context, 'helper.connected')
                    : t(context, 'home.helper.subtitle'),
                onTap: () => Navigator.push(
                  context,
                  MaterialPageRoute(builder: (_) => const HelperScreen()),
                ),
              );
            },
          ),
          StreamBuilder<VescTargetInfo>(
            stream: BleProxy.instance.vescTarget,
            initialData: BleProxy.instance.currentVescTarget,
            builder: (ctx, snap) {
              final target = snap.data;
              return _HubCard(
                icon: Icons.code,
                iconColor: (target?.connected ?? false) ? Colors.green : null,
                title: t(context, 'home.lisp.title'),
                subtitle: _targetLabel(context, target),
                onTap: () => Navigator.push(
                  context,
                  MaterialPageRoute(builder: (_) => const LispEditorScreen()),
                ),
              );
            },
          ),
          const SizedBox(height: 8),
          Card(
            child: ListTile(
              leading: const Icon(Icons.info_outline),
              title: Text(t(context, 'home.about.title')),
              trailing: const Icon(Icons.chevron_right),
              onTap: () => Navigator.push(
                context,
                MaterialPageRoute(builder: (_) => const AboutScreen()),
              ),
            ),
          ),
        ],
      ),
    );
  }

  /// Which VESC link the editor would use right now.
  String _targetLabel(BuildContext context, VescTargetInfo? target) {
    if (target == null) return t(context, 'lisp.adapter.none');
    if (target.kind == VescTargetKind.headUnit) {
      return target.headUnitAvailable
          ? t(context, 'lisp.adapter.headunit')
          : t(context, 'lisp.adapter.headunit.off');
    }
    final n = target.name;
    return (n == null || n.isEmpty)
        ? (target.remoteId ?? t(context, 'lisp.adapter.none'))
        : n;
  }

  void _showLanguagePicker(BuildContext context) {
    final notifier = LocaleScope.of(context);
    showModalBottomSheet<void>(
      context: context,
      builder: (ctx) => SafeArea(
        child: Column(
          mainAxisSize: MainAxisSize.min,
          children: [
            Padding(
              padding: const EdgeInsets.all(16),
              child: Text(t(context, 'lang.choose'),
                  style: Theme.of(context).textTheme.titleLarge),
            ),
            for (final l in supportedLocales)
              RadioListTile<String>(
                value: l.languageCode,
                groupValue: notifier.locale.languageCode,
                title: Text(t(context, 'lang.${l.languageCode}')),
                onChanged: (v) async {
                  if (v == null) return;
                  await notifier.set(Locale(v));
                  if (ctx.mounted) Navigator.pop(ctx);
                },
              ),
          ],
        ),
      ),
    );
  }
}

/// One of the two big entry points.
class _HubCard extends StatelessWidget {
  final IconData icon;
  final Color? iconColor;
  final String title;
  final String subtitle;
  final VoidCallback onTap;

  const _HubCard({
    required this.icon,
    this.iconColor,
    required this.title,
    required this.subtitle,
    required this.onTap,
  });

  @override
  Widget build(BuildContext context) {
    return Card(
      child: InkWell(
        onTap: onTap,
        child: Padding(
          padding: const EdgeInsets.symmetric(vertical: 12, horizontal: 16),
          child: Row(
            children: [
              Icon(icon, size: 40, color: iconColor),
              const SizedBox(width: 16),
              Expanded(
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    Text(title, style: Theme.of(context).textTheme.titleMedium),
                    const SizedBox(height: 4),
                    Text(subtitle,
                        style: Theme.of(context).textTheme.bodySmall),
                  ],
                ),
              ),
              const Icon(Icons.chevron_right),
            ],
          ),
        ),
      ),
    );
  }
}
