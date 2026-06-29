import 'package:flutter/material.dart';

import '../ble/ble_proxy.dart';
import '../bridge/notification_bridge.dart';
import '../i18n/strings.dart';
import '../settings/app_filter.dart';

class AppFilterScreen extends StatefulWidget {
  const AppFilterScreen({super.key});
  @override
  State<AppFilterScreen> createState() => _AppFilterScreenState();
}

class _AppFilterScreenState extends State<AppFilterScreen> {
  final _bridge = NotificationBridge();
  List<({String package, String label})> _apps = const [];
  AppFilter? _filter;
  bool _loading = true;
  String _query = '';

  @override
  void initState() {
    super.initState();
    _load();
  }

  Future<void> _load() async {
    final apps = await _bridge.listInstalledApps();
    // The pump (with its own AppFilter copy) lives in the background isolate
    // now; load our own copy here and tell it to reload after each edit.
    final filter = await AppFilter.load();
    if (!mounted) return;
    setState(() {
      _apps = apps;
      _filter = filter;
      _loading = false;
    });
  }

  @override
  Widget build(BuildContext context) {
    if (_loading) {
      return Scaffold(
        appBar: AppBar(title: Text(t(context, 'filter.title'))),
        body: const Center(child: CircularProgressIndicator()),
      );
    }
    final filtered = _query.isEmpty
        ? _apps
        : _apps
            .where((a) => a.label.toLowerCase().contains(_query.toLowerCase()))
            .toList();
    return Scaffold(
      appBar: AppBar(title: Text(t(context, 'filter.title'))),
      body: Column(
        children: [
          Padding(
            padding: const EdgeInsets.all(12),
            child: TextField(
              decoration: InputDecoration(
                prefixIcon: const Icon(Icons.search),
                hintText: t(context, 'filter.search'),
                border: const OutlineInputBorder(),
              ),
              onChanged: (v) => setState(() => _query = v),
            ),
          ),
          Expanded(
            child: ListView.builder(
              itemCount: filtered.length,
              itemBuilder: (_, i) {
                final a = filtered[i];
                final enabled = _filter!.allows(a.package);
                return CheckboxListTile(
                  title: Text(a.label),
                  subtitle: Text(a.package, style: const TextStyle(fontSize: 11)),
                  value: enabled,
                  onChanged: (v) async {
                    await _filter!.setEnabled(a.package, v ?? false);
                    // Push the change to the running pump in the BLE isolate.
                    BleProxy.instance.reloadFilter();
                    setState(() {});
                  },
                );
              },
            ),
          ),
        ],
      ),
    );
  }
}
