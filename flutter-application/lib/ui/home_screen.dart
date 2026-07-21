import 'dart:io';

import 'package:flutter/material.dart';
import 'package:permission_handler/permission_handler.dart';

import '../ble/ble_proxy.dart';
import '../bridge/notification_bridge.dart';
import '../i18n/strings.dart';
import 'about_screen.dart';
import 'app_filter_screen.dart';
import 'device_files_screen.dart';
import 'firmware_update_screen.dart';
import 'lisp_editor_screen.dart';
import 'pairing_screen.dart';
import 'splash_setup_screen.dart';
import 'test_panel_screen.dart';

class HomeScreen extends StatefulWidget {
  const HomeScreen({super.key});
  @override
  State<HomeScreen> createState() => _HomeScreenState();
}

class _HomeScreenState extends State<HomeScreen> {
  bool _notifPermission = false;
  bool _batteryUnrestricted = false;
  final _notifBridge = NotificationBridge();

  @override
  void initState() {
    super.initState();
    _refresh();
  }

  Future<void> _refresh() async {
    if (Platform.isAndroid) {
      final perm = await _notifBridge.hasPermission();
      final batt = await Permission.ignoreBatteryOptimizations.isGranted;
      if (mounted) {
        setState(() {
          _notifPermission = perm;
          _batteryUnrestricted = batt;
        });
      }
    }
  }

  Future<void> _requestBatteryOptOut() async {
    await Permission.ignoreBatteryOptimizations.request();
    Future.delayed(const Duration(seconds: 1), _refresh);
  }

  Future<void> _forget() async {
    final ok = await showDialog<bool>(
      context: context,
      builder: (ctx) => AlertDialog(
        title: Text(t(context, 'home.forget.confirm.title')),
        content: Text(t(context, 'home.forget.confirm.body')),
        actions: [
          TextButton(
              onPressed: () => Navigator.pop(ctx, false),
              child: Text(t(context, 'home.forget.cancel'))),
          FilledButton(
              onPressed: () => Navigator.pop(ctx, true),
              child: Text(t(context, 'home.forget.ok'))),
        ],
      ),
    );
    if (ok == true) {
      await BleProxy.instance.forget();
    }
  }

  Future<void> _restartBle() async {
    ScaffoldMessenger.of(context).showSnackBar(
      SnackBar(content: Text(t(context, 'home.restart.toast'))),
    );
    await BleProxy.instance.restartBle();
  }

  void _showLanguagePicker() {
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
                  if (mounted) Navigator.pop(ctx);
                },
              ),
          ],
        ),
      ),
    );
  }

  @override
  Widget build(BuildContext context) {
    final saved = BleProxy.instance.savedRemoteId;
    return Scaffold(
      appBar: AppBar(
        title: Text(t(context, 'app.title')),
        actions: [
          IconButton(
            icon: const Icon(Icons.translate),
            tooltip: t(context, 'home.lang.title'),
            onPressed: _showLanguagePicker,
          ),
          PopupMenuButton<String>(
            onSelected: (v) {
              if (v == 'test') {
                Navigator.push(
                  context,
                  MaterialPageRoute(builder: (_) => const TestPanelScreen()),
                );
              }
            },
            itemBuilder: (ctx) => [
              PopupMenuItem(
                value: 'test',
                child: Text(t(context, 'home.test.title')),
              ),
            ],
          ),
        ],
      ),
      body: ListView(
        padding: const EdgeInsets.all(16),
        children: [
          _StatusCard(onForget: _forget, onRestart: _restartBle),
          const SizedBox(height: 16),
          if (Platform.isAndroid) ...[
            Card(
              child: ListTile(
                leading: Icon(
                  _notifPermission ? Icons.check_circle : Icons.warning_amber,
                  color: _notifPermission ? Colors.green : Colors.orange,
                ),
                title: Text(t(context, 'home.notif.title')),
                subtitle: Text(_notifPermission
                    ? t(context, 'home.notif.granted')
                    : t(context, 'home.notif.denied')),
                trailing: const Icon(Icons.chevron_right),
                onTap: () async {
                  await _notifBridge.openPermissionSettings();
                  Future.delayed(const Duration(seconds: 1), _refresh);
                },
              ),
            ),
            Card(
              child: ListTile(
                leading: Icon(
                  _batteryUnrestricted
                      ? Icons.check_circle
                      : Icons.battery_alert,
                  color: _batteryUnrestricted ? Colors.green : Colors.orange,
                ),
                title: Text(t(context, 'home.batt.title')),
                subtitle: Text(_batteryUnrestricted
                    ? t(context, 'home.batt.granted')
                    : t(context, 'home.batt.denied')),
                trailing: const Icon(Icons.chevron_right),
                onTap: _requestBatteryOptOut,
              ),
            ),
          ] else
            Card(
              child: ListTile(
                leading: const Icon(Icons.info_outline),
                title: Text(t(context, 'home.ios.title')),
                subtitle: Text(t(context, 'home.ios.body')),
              ),
            ),
          const SizedBox(height: 8),
          Card(
            child: ListTile(
              leading: const Icon(Icons.bluetooth_searching),
              title: Text(t(context, 'home.pairing.title')),
              subtitle: Text(saved == null
                  ? t(context, 'home.pairing.none')
                  : '${t(context, 'home.pairing.saved')}$saved'),
              trailing: const Icon(Icons.chevron_right),
              onTap: () => Navigator.push(
                context,
                MaterialPageRoute(builder: (_) => const PairingScreen()),
              ),
            ),
          ),
          if (Platform.isAndroid)
            Card(
              child: ListTile(
                leading: const Icon(Icons.filter_alt_outlined),
                title: Text(t(context, 'home.filter.title')),
                trailing: const Icon(Icons.chevron_right),
                onTap: () => Navigator.push(
                  context,
                  MaterialPageRoute(builder: (_) => const AppFilterScreen()),
                ),
              ),
            ),
          if (Platform.isAndroid)
            Card(
              child: ListTile(
                leading: const Icon(Icons.system_update),
                title: Text(t(context, 'home.fw.title')),
                trailing: const Icon(Icons.chevron_right),
                onTap: () => Navigator.push(
                  context,
                  MaterialPageRoute(builder: (_) => const FirmwareUpdateScreen()),
                ),
              ),
            ),
          // Device file browser — only when connected to a head unit whose
          // firmware exposes the file-manager characteristics.
          StreamBuilder<BleConnState>(
            stream: BleProxy.instance.state,
            initialData: BleProxy.instance.currentState,
            builder: (ctx, snap) {
              final connected = snap.data == BleConnState.connected;
              if (!connected || !BleProxy.instance.supportsFileManager) {
                return const SizedBox.shrink();
              }
              return Column(
                children: [
                  Card(
                    child: ListTile(
                      leading: const Icon(Icons.folder_open),
                      title: Text(t(context, 'home.files.title')),
                      trailing: const Icon(Icons.chevron_right),
                      onTap: () => Navigator.push(
                        context,
                        MaterialPageRoute(
                            builder: (_) => const DeviceFilesScreen()),
                      ),
                    ),
                  ),
                  Card(
                    child: ListTile(
                      leading: const Icon(Icons.gif_box_outlined),
                      title: Text(t(context, 'home.splash.title')),
                      trailing: const Icon(Icons.chevron_right),
                      onTap: () => Navigator.push(
                        context,
                        MaterialPageRoute(
                            builder: (_) => const SplashSetupScreen()),
                      ),
                    ),
                  ),
                ],
              );
            },
          ),
          // LISP editor — only when connected to a head unit whose firmware
          // exposes the NUS bridge (VESC-Tool-compatible transport).
          StreamBuilder<BleConnState>(
            stream: BleProxy.instance.state,
            initialData: BleProxy.instance.currentState,
            builder: (ctx, snap) {
              final connected = snap.data == BleConnState.connected;
              if (!connected || !BleProxy.instance.supportsLisp) {
                return const SizedBox.shrink();
              }
              return Card(
                child: ListTile(
                  leading: const Icon(Icons.code),
                  title: Text(t(context, 'home.lisp.title')),
                  trailing: const Icon(Icons.chevron_right),
                  onTap: () => Navigator.push(
                    context,
                    MaterialPageRoute(
                        builder: (_) => const LispEditorScreen()),
                  ),
                ),
              );
            },
          ),
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
}

class _StatusCard extends StatelessWidget {
  final VoidCallback onForget;
  final VoidCallback onRestart;
  const _StatusCard({required this.onForget, required this.onRestart});

  @override
  Widget build(BuildContext context) {
    return StreamBuilder<BleConnState>(
      stream: BleProxy.instance.state,
      initialData: BleProxy.instance.currentState,
      builder: (ctx, snap) {
        final state = snap.data ?? BleConnState.idle;
        final connected = state == BleConnState.connected;
        final hasSaved = BleProxy.instance.savedRemoteId != null;
        return Card(
          color: connected
              ? Colors.green.withValues(alpha: 0.15)
              : Theme.of(context).colorScheme.surfaceContainerHigh,
          child: Column(
            children: [
              ListTile(
                leading: Icon(
                  connected
                      ? Icons.bluetooth_connected
                      : Icons.bluetooth_disabled,
                  color: connected ? Colors.green : Colors.grey,
                  size: 32,
                ),
                title: Text(connected
                    ? t(context, 'home.status.connected')
                    : t(context, 'home.status.disconnected')),
                subtitle: Text(_label(context, state)),
              ),
              if (hasSaved)
                Padding(
                  padding: const EdgeInsets.only(right: 8, bottom: 4),
                  child: Row(
                    mainAxisAlignment: MainAxisAlignment.end,
                    children: [
                      TextButton.icon(
                        onPressed: onRestart,
                        icon: const Icon(Icons.restart_alt, size: 18),
                        label: Text(t(context, 'home.restart')),
                      ),
                      TextButton.icon(
                        onPressed: onForget,
                        icon: const Icon(Icons.link_off, size: 18),
                        label: Text(t(context, 'home.forget')),
                      ),
                    ],
                  ),
                ),
            ],
          ),
        );
      },
    );
  }

  String _label(BuildContext c, BleConnState s) => switch (s) {
        BleConnState.idle => t(c, 'home.state.idle'),
        BleConnState.scanning => t(c, 'home.state.scanning'),
        BleConnState.connecting => t(c, 'home.state.connecting'),
        BleConnState.connected => t(c, 'home.state.connected'),
        BleConnState.disconnected => t(c, 'home.state.disconnected'),
      };
}
