import 'dart:io';

import 'package:flutter/material.dart';
import 'package:permission_handler/permission_handler.dart';

import '../ble/ble_service.dart';
import '../bridge/notification_bridge.dart';
import 'app_filter_screen.dart';
import 'pairing_screen.dart';

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
        title: const Text('Забыть устройство?'),
        content: const Text(
            'Авто-подключение отключится. Чтобы снова подключиться, '
            'придётся пройти pairing заново.'),
        actions: [
          TextButton(
              onPressed: () => Navigator.pop(ctx, false),
              child: const Text('Отмена')),
          FilledButton(
              onPressed: () => Navigator.pop(ctx, true),
              child: const Text('Забыть')),
        ],
      ),
    );
    if (ok == true) {
      await BleService.instance.forget();
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: const Text('AA Bridge')),
      body: ListView(
        padding: const EdgeInsets.all(16),
        children: [
          _StatusCard(onForget: _forget),
          const SizedBox(height: 16),
          if (Platform.isAndroid) ...[
            Card(
              child: ListTile(
                leading: Icon(
                  _notifPermission ? Icons.check_circle : Icons.warning_amber,
                  color: _notifPermission ? Colors.green : Colors.orange,
                ),
                title: const Text('Доступ к уведомлениям'),
                subtitle: Text(_notifPermission
                    ? 'Разрешено'
                    : 'Нужно разрешить в системных настройках'),
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
                  _batteryUnrestricted ? Icons.check_circle : Icons.battery_alert,
                  color: _batteryUnrestricted ? Colors.green : Colors.orange,
                ),
                title: const Text('Без энергосбережения'),
                subtitle: Text(_batteryUnrestricted
                    ? 'ОС не будет душить BLE-соединение'
                    : 'Без этого ОС отключит BLE через ~30 мин в фоне'),
                trailing: const Icon(Icons.chevron_right),
                onTap: _requestBatteryOptOut,
              ),
            ),
          ] else
            const Card(
              child: ListTile(
                leading: Icon(Icons.info_outline),
                title: Text('iOS режим'),
                subtitle: Text(
                    'На iOS уведомления приложений не пересылаются (sandbox). '
                    'Доступно только pairing и media-метаданные собственного '
                    'приложения.'),
              ),
            ),
          const SizedBox(height: 8),
          Card(
            child: ListTile(
              leading: const Icon(Icons.bluetooth_searching),
              title: const Text('Pairing с head unit'),
              subtitle: Text(BleService.instance.savedRemoteId == null
                  ? 'Устройство не выбрано'
                  : 'Сохранено: ${BleService.instance.savedRemoteId}'),
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
                title: const Text('Какие приложения транслировать'),
                trailing: const Icon(Icons.chevron_right),
                onTap: () => Navigator.push(
                  context,
                  MaterialPageRoute(builder: (_) => const AppFilterScreen()),
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
  const _StatusCard({required this.onForget});

  @override
  Widget build(BuildContext context) {
    return StreamBuilder<BleConnState>(
      stream: BleService.instance.state,
      initialData: BleService.instance.currentState,
      builder: (context, snap) {
        final state = snap.data ?? BleConnState.idle;
        final connected = state == BleConnState.connected;
        final hasSaved = BleService.instance.savedRemoteId != null;
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
                title: Text(connected ? 'Подключено' : 'Не подключено'),
                subtitle: Text(_label(state)),
              ),
              if (hasSaved)
                Padding(
                  padding: const EdgeInsets.only(right: 8, bottom: 4),
                  child: Row(
                    mainAxisAlignment: MainAxisAlignment.end,
                    children: [
                      TextButton.icon(
                        onPressed: onForget,
                        icon: const Icon(Icons.link_off, size: 18),
                        label: const Text('Забыть устройство'),
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

  String _label(BleConnState s) => switch (s) {
        BleConnState.idle => 'BLE простой',
        BleConnState.scanning => 'Сканирую…',
        BleConnState.connecting => 'Подключаюсь…',
        BleConnState.connected => 'Соединение установлено',
        BleConnState.disconnected =>
          'Соединение потеряно, жду пока устройство появится в эфире…',
      };
}
