import 'package:flutter/material.dart';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';
import 'package:permission_handler/permission_handler.dart';

import '../ble/ble_service.dart';
import '../i18n/strings.dart';

class PairingScreen extends StatefulWidget {
  const PairingScreen({super.key});
  @override
  State<PairingScreen> createState() => _PairingScreenState();
}

class _PairingScreenState extends State<PairingScreen> {
  List<ScanResult> _results = [];
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
      debugPrint('[pairing] perms granted: $perms');
      final r = await BleService.instance.scan();
      debugPrint('[pairing] scan returned ${r.length} results');
      if (!mounted) return;
      setState(() => _results = r);
    } catch (e, st) {
      debugPrint('[pairing] scan FAILED: $e\n$st');
      if (mounted) setState(() => _error = '$e');
    } finally {
      if (mounted) setState(() => _scanning = false);
    }
  }

  Future<void> _connect(BluetoothDevice d) async {
    debugPrint('[pairing] connect → ${d.remoteId} (${d.platformName})');
    try {
      await BleService.instance.connect(d);
      debugPrint('[pairing] connect OK');
      if (!mounted) return;
      Navigator.pop(context);
    } catch (e, st) {
      debugPrint('[pairing] connect FAILED: $e\n$st');
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
                final name = r.advertisementData.advName.isNotEmpty
                    ? r.advertisementData.advName
                    : (r.device.platformName.isNotEmpty
                        ? r.device.platformName
                        : t(context, 'pairing.unnamed'));
                return ListTile(
                  leading: const Icon(Icons.bluetooth),
                  title: Text(name),
                  subtitle: Text('${r.device.remoteId} · rssi ${r.rssi} dBm'),
                  onTap: () => _connect(r.device),
                );
              },
            ),
          ),
        ],
      ),
    );
  }
}
