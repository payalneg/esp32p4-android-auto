import 'dart:io';

import 'package:flutter/material.dart';

import '../ble/ble_service.dart';
import '../bridge/foreground_bridge.dart';
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
  final _notifBridge = NotificationBridge();
  final _fg = ForegroundBridge();

  @override
  void initState() {
    super.initState();
    _refresh();
  }

  Future<void> _refresh() async {
    if (Platform.isAndroid) {
      final p = await _notifBridge.hasPermission();
      if (mounted) setState(() => _notifPermission = p);
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: const Text('AA Bridge')),
      body: ListView(
        padding: const EdgeInsets.all(16),
        children: [
          _StatusCard(),
          const SizedBox(height: 16),
          if (Platform.isAndroid)
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
                  // Reload when user comes back.
                  Future.delayed(const Duration(seconds: 1), _refresh);
                },
              ),
            )
          else
            const Card(
              child: ListTile(
                leading: Icon(Icons.info_outline),
                title: Text('iOS режим'),
                subtitle: Text(
                    'На iOS уведомления приложений не пересылаются (sandbox). '
                    'Доступно только pairing.'),
              ),
            ),
          const SizedBox(height: 8),
          Card(
            child: ListTile(
              leading: const Icon(Icons.bluetooth_searching),
              title: const Text('Pairing с head unit'),
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
          const SizedBox(height: 16),
          ElevatedButton.icon(
            onPressed: () => _fg.start(),
            icon: const Icon(Icons.play_arrow),
            label: const Text('Запустить фоновую службу'),
          ),
          const SizedBox(height: 8),
          OutlinedButton.icon(
            onPressed: () => _fg.stop(),
            icon: const Icon(Icons.stop),
            label: const Text('Остановить фоновую службу'),
          ),
        ],
      ),
    );
  }
}

class _StatusCard extends StatelessWidget {
  @override
  Widget build(BuildContext context) {
    return StreamBuilder<BleConnState>(
      stream: BleService.instance.state,
      initialData: BleService.instance.currentState,
      builder: (context, snap) {
        final state = snap.data ?? BleConnState.idle;
        final connected = state == BleConnState.connected;
        return Card(
          color: connected
              ? Colors.green.withValues(alpha: 0.15)
              : Theme.of(context).colorScheme.surfaceContainerHigh,
          child: ListTile(
            leading: Icon(
              connected ? Icons.bluetooth_connected : Icons.bluetooth_disabled,
              color: connected ? Colors.green : Colors.grey,
              size: 32,
            ),
            title: Text(connected ? 'Подключено' : 'Не подключено'),
            subtitle: Text(_label(state)),
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
        BleConnState.disconnected => 'Соединение потеряно',
      };
}
