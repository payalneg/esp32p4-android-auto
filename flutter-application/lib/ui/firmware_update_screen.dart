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

  /// Board model the head unit reports over BLE (null when it can't be read).
  String? _deviceModel;

  /// Board whose image will actually be flashed. Defaults to [_deviceModel]
  /// when it's a known board, else [FirmwareUpdater.defaultModel]; the user
  /// can override it with the picker.
  String? _selectedModel;
  String? _bundled;

  /// Whether the head unit can be flashed over BLE (firmware exposes the OTA
  /// data characteristics). Drives whether the Bluetooth method is offered.
  bool _bleOtaSupported = false;

  /// Chosen update transport. Defaults to BLE when the head unit supports it
  /// (no WiFi juggling), else WiFi.
  OtaTransport _transport = OtaTransport.wifi;
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
    final detected = info?.model;
    final bleOta = BleService.instance.supportsBleOta;
    setState(() {
      _bundled = bundled;
      _deviceVersion = info?.version;
      _deviceModel = detected;
      _selectedModel = FirmwareUpdater.boards.contains(detected)
          ? detected
          : FirmwareUpdater.defaultModel;
      _bleOtaSupported = bleOta;
      _transport = bleOta ? OtaTransport.ble : OtaTransport.wifi;
      _loading = false;
    });
  }

  Future<void> _confirmAndFlash() async {
    final viaBle = _transport == OtaTransport.ble;
    final host = _hostCtrl.text.trim();
    if (!viaBle && host.isEmpty) return;
    final boardLine = '${t(context, 'fw.select')}: '
        '${FirmwareUpdater.displayName(_selectedModel)}';
    final methodLine = '${t(context, 'fw.method')}: '
        '${t(context, viaBle ? 'fw.method.ble' : 'fw.method.wifi')}';
    final go = await showDialog<bool>(
      context: context,
      builder: (ctx) => AlertDialog(
        title: Text(t(ctx, 'fw.warn.title')),
        content: Text('${t(ctx, 'fw.warn.body')}\n\n$methodLine\n$boardLine'),
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
    if (viaBle) {
      await _updater.runBle(model: _selectedModel);
    } else {
      await _updater.run(host: host, model: _selectedModel);
    }
    if (mounted) setState(() => _busy = false);
  }

  /// Warning shown under the firmware picker, or null when the selection is
  /// trustworthy. Two cases: the head unit's board couldn't be read at all, or
  /// the user picked an image for a different board than the one detected.
  String? _modelWarning(BuildContext context) {
    if (_deviceModel == null) return t(context, 'fw.warn.undetected');
    if (FirmwareUpdater.boards.contains(_deviceModel) &&
        _selectedModel != _deviceModel) {
      return t(context, 'fw.warn.mismatch');
    }
    return null;
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: Text(t(context, 'fw.title'))),
      // SafeArea keeps the bottom "Flash" button clear of the system
      // navigation bar / gesture pill (the Column pins it to the bottom with a
      // Spacer, so without this it sits under the nav buttons).
      body: _loading
          ? const Center(child: CircularProgressIndicator())
          : SafeArea(
              child: Padding(
                padding: const EdgeInsets.all(16),
                child: _body(context),
              ),
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
                leading: const Icon(Icons.developer_board),
                title: Text(t(context, 'fw.detected')),
                trailing: Text(
                    _deviceModel != null
                        ? FirmwareUpdater.displayName(_deviceModel)
                        : t(context, 'fw.detected.unknown'),
                    style: Theme.of(context).textTheme.titleMedium?.copyWith(
                          color: _deviceModel == null ? Colors.orange : null,
                        )),
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
        DropdownButtonFormField<String>(
          value: _selectedModel,
          decoration: InputDecoration(
            labelText: t(context, 'fw.select'),
            border: const OutlineInputBorder(),
            prefixIcon: const Icon(Icons.system_update_alt),
          ),
          items: [
            for (final b in FirmwareUpdater.boards)
              DropdownMenuItem(
                value: b,
                child: Text(FirmwareUpdater.displayName(b)),
              ),
          ],
          onChanged:
              _busy ? null : (v) => setState(() => _selectedModel = v),
        ),
        if (_modelWarning(context) case final w?) ...[
          const SizedBox(height: 6),
          Row(
            children: [
              const Icon(Icons.warning_amber, size: 16, color: Colors.orange),
              const SizedBox(width: 6),
              Expanded(
                child: Text(w,
                    style:
                        const TextStyle(color: Colors.orange, fontSize: 13)),
              ),
            ],
          ),
        ],
        const SizedBox(height: 16),
        Align(
          alignment: Alignment.centerLeft,
          child: Text(t(context, 'fw.method'),
              style: Theme.of(context).textTheme.labelLarge),
        ),
        const SizedBox(height: 6),
        SegmentedButton<OtaTransport>(
          segments: [
            ButtonSegment(
              value: OtaTransport.ble,
              icon: const Icon(Icons.bluetooth),
              label: Text(t(context, 'fw.method.ble')),
              enabled: _bleOtaSupported,
            ),
            ButtonSegment(
              value: OtaTransport.wifi,
              icon: const Icon(Icons.wifi),
              label: Text(t(context, 'fw.method.wifi')),
            ),
          ],
          selected: {_transport},
          onSelectionChanged:
              _busy ? null : (s) => setState(() => _transport = s.first),
        ),
        const SizedBox(height: 6),
        Row(
          children: [
            Icon(Icons.info_outline, size: 16, color: Colors.grey[600]),
            const SizedBox(width: 6),
            Expanded(
              child: Text(
                  _transport == OtaTransport.ble
                      ? t(context, 'fw.method.ble.note')
                      : (_bleOtaSupported
                          ? t(context, 'fw.host.note')
                          : t(context, 'fw.method.ble.unsupported')),
                  style: TextStyle(color: Colors.grey[600], fontSize: 13)),
            ),
          ],
        ),
        if (_transport == OtaTransport.wifi) ...[
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
        ],
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
        if (_localizeMsg(context, s) case final msg?)
          Text(
            msg,
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

  /// Localise an [UpdateState]'s message key and fill its {placeholder}s. The
  /// updater layers emit i18n keys (no BuildContext there); the text is built
  /// here so it honours the selected app language.
  String? _localizeMsg(BuildContext context, UpdateState s) {
    final key = s.messageKey;
    if (key == null) return null;
    var text = t(context, key);
    s.args?.forEach((k, v) => text = text.replaceAll('{$k}', v));
    return text;
  }
}
