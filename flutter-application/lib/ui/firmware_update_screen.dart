import 'package:flutter/material.dart';

import '../ble/ble_service.dart';
import '../firmware/firmware_updater.dart';
import '../firmware/ota_info.dart';
import '../i18n/strings.dart';

/// Over-WiFi firmware update screen. Prefers reading the head unit's SoftAP
/// credentials + version over BLE; if that's unavailable (older firmware, BLE
/// down, or empty password) it falls back to manual SSID/password entry and
/// flashes the firmware image bundled in the APK either way.
class FirmwareUpdateScreen extends StatefulWidget {
  const FirmwareUpdateScreen({super.key});

  @override
  State<FirmwareUpdateScreen> createState() => _FirmwareUpdateScreenState();
}

class _FirmwareUpdateScreenState extends State<FirmwareUpdateScreen> {
  final _updater = FirmwareUpdater();
  final _ssidCtrl = TextEditingController();
  final _passCtrl = TextEditingController();

  OtaInfo? _info;
  String? _bundled;
  bool _loading = true;
  bool _busy = false;
  UpdateState _state = const UpdateState(UpdatePhase.idle);

  /// True when BLE handed us a complete SoftAP credential set — otherwise we
  /// show the manual SSID/password form.
  bool get _hasAutoCreds =>
      _info != null && _info!.ssid.isNotEmpty && _info!.password.isNotEmpty;

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
    _ssidCtrl.dispose();
    _passCtrl.dispose();
    super.dispose();
  }

  Future<void> _load() async {
    final bundled = await FirmwareUpdater.bundledVersion();
    final info = BleService.instance.supportsOta
        ? await BleService.instance.readOtaInfo()
        : null;
    if (!mounted) return;
    setState(() {
      _bundled = bundled;
      _info = info;
      // Prefill whatever BLE did give us so the user only fills the gaps.
      if (info != null && info.ssid.isNotEmpty) _ssidCtrl.text = info.ssid;
      if (info != null && info.password.isNotEmpty) _passCtrl.text = info.password;
      _loading = false;
    });
  }

  /// Build the OtaInfo to flash with — from BLE when complete, else from the
  /// manual fields (with sensible IP/port defaults). Returns null if manual
  /// fields are incomplete.
  OtaInfo? _resolveTarget() {
    if (_hasAutoCreds) return _info;
    final ssid = _ssidCtrl.text.trim();
    final pass = _passCtrl.text;
    if (ssid.isEmpty || pass.isEmpty) return null;
    return OtaInfo(
      ip: _info?.ip.isNotEmpty == true ? _info!.ip : '192.168.4.1',
      port: _info?.port ?? 80,
      ssid: ssid,
      password: pass,
      version: _info?.version ?? '',
    );
  }

  Future<void> _confirmAndFlash() async {
    final target = _resolveTarget();
    if (target == null) {
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(content: Text(t(context, 'fw.manual.needfields'))),
      );
      return;
    }
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
    await _updater.run(target);
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
    final deviceVer = _info?.version;
    final upToDate = deviceVer != null && deviceVer == bundled;

    return Column(
      crossAxisAlignment: CrossAxisAlignment.stretch,
      children: [
        Card(
          child: Column(
            children: [
              ListTile(
                leading: const Icon(Icons.memory),
                title: Text(t(context, 'fw.device')),
                trailing: Text(deviceVer ?? '—',
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
        const SizedBox(height: 12),
        if (upToDate)
          Text(t(context, 'fw.uptodate'),
              style: TextStyle(color: Colors.grey[600])),
        if (!_hasAutoCreds) _manualForm(context),
        const SizedBox(height: 8),
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

  Widget _manualForm(BuildContext context) {
    return Column(
      crossAxisAlignment: CrossAxisAlignment.stretch,
      children: [
        const SizedBox(height: 4),
        Row(
          children: [
            const Icon(Icons.wifi_find, size: 18, color: Colors.orange),
            const SizedBox(width: 8),
            Expanded(
              child: Text(t(context, 'fw.manual.hint'),
                  style: TextStyle(color: Colors.grey[600], fontSize: 13)),
            ),
          ],
        ),
        const SizedBox(height: 8),
        TextField(
          controller: _ssidCtrl,
          enabled: !_busy,
          decoration: InputDecoration(
            labelText: t(context, 'fw.manual.ssid'),
            border: const OutlineInputBorder(),
            prefixIcon: const Icon(Icons.wifi),
          ),
        ),
        const SizedBox(height: 12),
        TextField(
          controller: _passCtrl,
          enabled: !_busy,
          obscureText: true,
          decoration: InputDecoration(
            labelText: t(context, 'fw.manual.password'),
            border: const OutlineInputBorder(),
            prefixIcon: const Icon(Icons.password),
          ),
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
