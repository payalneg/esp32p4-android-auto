import 'dart:io' show Platform;

import 'package:flutter/material.dart';
import 'package:permission_handler/permission_handler.dart';

import '../ble/ble_proxy.dart';
import '../bridge/foreground_bridge.dart';
import '../i18n/strings.dart';
import '../log/app_log.dart';

class PairingScreen extends StatefulWidget {
  const PairingScreen({super.key});
  @override
  State<PairingScreen> createState() => _PairingScreenState();
}

class _PairingScreenState extends State<PairingScreen> {
  List<ScanDevice> _results = [];
  bool _scanning = false;
  String? _error;

  Future<void> _scan() async {
    setState(() {
      _error = null;
      _scanning = true;
    });
    try {
      final perms = await [
        Permission.bluetoothScan,
        Permission.bluetoothConnect,
        Permission.locationWhenInUse,
      ].request();
      AppLog.i('pair', 'permissions granted: $perms');
      // Make sure the background BLE isolate is running before we talk to it:
      // if BLUETOOTH_CONNECT was denied at app launch, the foreground service
      // (and the task that owns BLE) was never started. Idempotent.
      if (Platform.isAndroid &&
          await Permission.bluetoothConnect.isGranted) {
        await ForegroundBridge().start();
      }
      // Scan runs in the background BLE isolate; we get back serializable hits.
      final r = await BleProxy.instance.scan();
      AppLog.i('pair', 'scan returned ${r.length} devices');
      if (!mounted) return;
      setState(() => _results = r);
    } catch (e) {
      AppLog.e('pair', 'scan failed: $e');
      if (mounted) setState(() => _error = '$e');
    } finally {
      if (mounted) setState(() => _scanning = false);
    }
  }

  Future<void> _connect(ScanDevice d) async {
    AppLog.i('pair', 'connecting to ${d.name} (${d.remoteId})');
    try {
      await BleProxy.instance.connect(d.remoteId);
      AppLog.i('pair', 'connected');
      if (!mounted) return;
      Navigator.pop(context);
    } catch (e) {
      AppLog.e('pair', 'connect failed: $e');
      if (!mounted) return;
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(content: Text('${t(context, 'pairing.error')}$e')),
      );
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: Text(t(context, 'pairing.title')),
        actions: [
          IconButton(
            onPressed: _scanning ? null : _scan,
            icon: const Icon(Icons.refresh),
          ),
        ],
      ),
      body: Column(
        children: [
          if (_scanning) const LinearProgressIndicator(),
          if (_error != null)
            Padding(
              padding: const EdgeInsets.all(16),
              child: Text(_error!, style: const TextStyle(color: Colors.red)),
            ),
          if (!_scanning && _results.isEmpty)
            Padding(
              padding: const EdgeInsets.all(24),
              child: Text(
                t(context, 'pairing.empty'),
                textAlign: TextAlign.center,
              ),
            ),
          Expanded(
            child: ListView.builder(
              itemCount: _results.length,
              itemBuilder: (_, i) {
                final r = _results[i];
                final name =
                    r.name.isNotEmpty ? r.name : t(context, 'pairing.unnamed');
                return ListTile(
                  leading: const Icon(Icons.bluetooth),
                  title: Text(name),
                  subtitle: Text('${r.remoteId} · rssi ${r.rssi} dBm'),
                  onTap: () => _connect(r),
                );
              },
            ),
          ),
        ],
      ),
    );
  }
}
