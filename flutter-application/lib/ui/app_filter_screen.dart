import 'package:flutter/material.dart';

import '../bridge/notification_bridge.dart';
import '../coordinator.dart';
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
    final filter = Coordinator.instance.filter ?? await AppFilter.load();
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
        appBar: AppBar(title: const Text('Приложения')),
        body: const Center(child: CircularProgressIndicator()),
      );
    }
    final filtered = _query.isEmpty
        ? _apps
        : _apps
            .where((a) => a.label.toLowerCase().contains(_query.toLowerCase()))
            .toList();
    return Scaffold(
      appBar: AppBar(title: const Text('Приложения')),
      body: Column(
        children: [
          Padding(
            padding: const EdgeInsets.all(12),
            child: TextField(
              decoration: const InputDecoration(
                prefixIcon: Icon(Icons.search),
                hintText: 'Поиск…',
                border: OutlineInputBorder(),
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
