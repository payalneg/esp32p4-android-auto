import 'package:flutter/material.dart';

import '../ble/ble_service.dart';
import '../firmware/firmware_updater.dart';
import '../i18n/strings.dart';

/// Firmware update screen. Uploads the firmware image bundled in the APK to
/// the head unit's HTTP OTA endpoint over WiFi. The phone is assumed to be on
/// the head unit's network already; we reach it by mDNS name (editable as a
/// fallback). Device version is shown over BLE when connected, for context.
class FirmwareUpdateScreen extends StatefulWidget {
  const FirmwareUpdateScreen({super.key});

  @override
  State<FirmwareUpdateScreen> createState() => _FirmwareUpdateScreenState();
}

class _FirmwareUpdateScreenState extends State<FirmwareUpdateScreen> {
  final _updater = FirmwareUpdater();
  final _hostCtrl = TextEditingController(text: FirmwareUpdater.defaultHost);

  String? _deviceVersion;
  String? _bundled;
  bool _loading = true;
  bool _busy = false;
  UpdateState _state = const UpdateState(UpdatePhase.idle);

  @override
  void initState() {
    super.initState();
    _updater.state.listen((s) {
      if (mounted) setState(() => _state = s);
    });
    _load();
  }

  @override
  void dispose() {
    _updater.dispose();
    _hostCtrl.dispose();
    super.dispose();
  }

  Future<void> _load() async {
    final bundled = await FirmwareUpdater.bundledVersion();
    // Best-effort: show the running version if BLE exposes it. Not required
    // for flashing.
    final info = BleService.instance.supportsOta
        ? await BleService.instance.readOtaInfo()
        : null;
    if (!mounted) return;
    setState(() {
      _bundled = bundled;
      _deviceVersion = info?.version;
      _loading = false;
    });
  }

  Future<void> _confirmAndFlash() async {
    final host = _hostCtrl.text.trim();
    if (host.isEmpty) return;
    final go = await showDialog<bool>(
      context: context,
      builder: (ctx) => AlertDialog(
        title: Text(t(ctx, 'fw.warn.title')),
        content: Text(t(ctx, 'fw.warn.body')),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(ctx, false),
            child: Text(t(ctx, 'fw.cancel')),
          ),
          FilledButton(
            onPressed: () => Navigator.pop(ctx, true),
            child: Text(t(ctx, 'fw.warn.go')),
          ),
        ],
      ),
    );
    if (go != true) return;
    setState(() => _busy = true);
    await _updater.run(host: host);
    if (mounted) setState(() => _busy = false);
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: Text(t(context, 'fw.title'))),
      body: _loading
          ? const Center(child: CircularProgressIndicator())
          : Padding(
              padding: const EdgeInsets.all(16),
              child: _body(context),
            ),
    );
  }

  Widget _body(BuildContext context) {
    final bundled = _bundled ?? '?';
    final upToDate = _deviceVersion != null && _deviceVersion == bundled;

    return Column(
      crossAxisAlignment: CrossAxisAlignment.stretch,
      children: [
        Card(
          child: Column(
            children: [
              ListTile(
                leading: const Icon(Icons.memory),
                title: Text(t(context, 'fw.device')),
                trailing: Text(_deviceVersion ?? '—',
                    style: Theme.of(context).textTheme.titleMedium),
              ),
              const Divider(height: 1),
              ListTile(
                leading: const Icon(Icons.phone_android),
                title: Text(t(context, 'fw.bundled')),
                trailing: Text(bundled,
                    style: Theme.of(context).textTheme.titleMedium?.copyWith(
                          color: upToDate ? null : Colors.green,
                        )),
              ),
            ],
          ),
        ),
        const SizedBox(height: 16),
        TextField(
          controller: _hostCtrl,
          enabled: !_busy,
          decoration: InputDecoration(
            labelText: t(context, 'fw.host'),
            border: const OutlineInputBorder(),
            prefixIcon: const Icon(Icons.lan),
          ),
        ),
        const SizedBox(height: 6),
        Row(
          children: [
            Icon(Icons.info_outline, size: 16, color: Colors.grey[600]),
            const SizedBox(width: 6),
            Expanded(
              child: Text(t(context, 'fw.host.note'),
                  style: TextStyle(color: Colors.grey[600], fontSize: 13)),
            ),
          ],
        ),
        if (upToDate) ...[
          const SizedBox(height: 12),
          Text(t(context, 'fw.uptodate'),
              style: TextStyle(color: Colors.grey[600])),
        ],
        const SizedBox(height: 12),
        _progress(context),
        const Spacer(),
        FilledButton.icon(
          onPressed: _busy ? null : _confirmAndFlash,
          icon: _busy
              ? const SizedBox(
                  width: 18,
                  height: 18,
                  child: CircularProgressIndicator(strokeWidth: 2))
              : const Icon(Icons.system_update),
          label: Text(_busy ? t(context, 'fw.flashing') : t(context, 'fw.flash')),
        ),
      ],
    );
  }

  Widget _progress(BuildContext context) {
    final s = _state;
    if (s.phase == UpdatePhase.idle) return const SizedBox.shrink();
    final showBar = s.phase == UpdatePhase.uploading;
    final isError = s.phase == UpdatePhase.error;
    final isDone = s.phase == UpdatePhase.done;
    return Column(
      crossAxisAlignment: CrossAxisAlignment.stretch,
      children: [
        if (showBar) ...[
          LinearProgressIndicator(value: s.progress),
          const SizedBox(height: 4),
          Text('${(s.progress * 100).round()}%', textAlign: TextAlign.center),
          const SizedBox(height: 8),
        ],
        if (s.message != null)
          Text(
            isDone ? t(context, 'fw.done') : s.message!,
            textAlign: TextAlign.center,
            style: TextStyle(
              color: isError
                  ? Theme.of(context).colorScheme.error
                  : isDone
                      ? Colors.green
                      : null,
            ),
          ),
      ],
    );
  }
}
