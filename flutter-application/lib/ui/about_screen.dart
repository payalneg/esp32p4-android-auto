import 'package:flutter/material.dart';
import 'package:package_info_plus/package_info_plus.dart';

import '../firmware/firmware_updater.dart';
import '../i18n/strings.dart';

/// Simple About screen: app icon, app version (from the build), bundled
/// firmware version, a short description and the source-code URL.
class AboutScreen extends StatefulWidget {
  const AboutScreen({super.key});

  @override
  State<AboutScreen> createState() => _AboutScreenState();
}

class _AboutScreenState extends State<AboutScreen> {
  String _appVersion = '…';
  String _fwVersion = '…';

  static const _sourceUrl = 'github.com/payalneg/esp32p4-android-auto';

  @override
  void initState() {
    super.initState();
    _load();
  }

  Future<void> _load() async {
    String app = '?';
    try {
      final info = await PackageInfo.fromPlatform();
      app = '${info.version} (${info.buildNumber})';
    } catch (_) {}
    String fw = '?';
    try {
      fw = await FirmwareUpdater.bundledVersion();
    } catch (_) {}
    if (!mounted) return;
    setState(() {
      _appVersion = app;
      _fwVersion = fw;
    });
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: Text(t(context, 'about.title'))),
      body: ListView(
        padding: const EdgeInsets.all(16),
        children: [
          const SizedBox(height: 8),
          Center(
            child: ClipRRect(
              borderRadius: BorderRadius.circular(20),
              child: Image.asset('assets/icon_legacy.png',
                  width: 96, height: 96, fit: BoxFit.cover),
            ),
          ),
          const SizedBox(height: 12),
          Center(
            child: Text('AA Bridge',
                style: Theme.of(context).textTheme.headlineSmall),
          ),
          const SizedBox(height: 24),
          Card(
            child: Column(
              children: [
                ListTile(
                  leading: const Icon(Icons.phone_android),
                  title: Text(t(context, 'about.app')),
                  trailing: Text(_appVersion,
                      style: Theme.of(context).textTheme.titleMedium),
                ),
                const Divider(height: 1),
                ListTile(
                  leading: const Icon(Icons.memory),
                  title: Text(t(context, 'about.fw')),
                  trailing: Text(_fwVersion,
                      style: Theme.of(context).textTheme.titleMedium),
                ),
              ],
            ),
          ),
          const SizedBox(height: 16),
          Text(t(context, 'about.desc'),
              style: TextStyle(color: Colors.grey[600])),
          const SizedBox(height: 16),
          Card(
            child: ListTile(
              leading: const Icon(Icons.code),
              title: Text(t(context, 'about.source')),
              subtitle: const Text(_sourceUrl),
            ),
          ),
        ],
      ),
    );
  }
}
