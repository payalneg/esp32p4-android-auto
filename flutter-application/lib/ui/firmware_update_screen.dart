import 'dart:async';
import 'dart:io';
import 'dart:typed_data';

import 'package:file_picker/file_picker.dart';
import 'package:flutter/material.dart';

import 'recovery_screen.dart';

import '../ble/ble_proxy.dart';
import '../firmware/esp_image.dart';
import '../firmware/firmware_updater.dart';
import '../firmware/usb_flasher.dart';
import '../firmware/usb_serial_link.dart';
import '../i18n/strings.dart';

/// Firmware update screen.
///
/// Three ways to get an image onto the head unit:
///  * WiFi  — HTTP OTA to the running firmware (needs the head unit's network);
///  * BLE   — streamed over the existing Bluetooth link (slower, no network);
///  * USB   — the ROM serial bootloader over a USB cable, for the cases the
///            other two cannot cover (device doesn't boot). The USB path can
///            flash the image bundled in the APK or any .bin the user picks,
///            including a merged recovery image.
class FirmwareUpdateScreen extends StatefulWidget {
  const FirmwareUpdateScreen({super.key});

  @override
  State<FirmwareUpdateScreen> createState() => _FirmwareUpdateScreenState();
}

enum _UsbSource { bundled, file }

class _FirmwareUpdateScreenState extends State<FirmwareUpdateScreen> {
  final _updater = FirmwareUpdater();
  final _usb = UsbFlasher();
  final _hostCtrl = TextEditingController(text: FirmwareUpdater.defaultHost);
  final _offsetCtrl = TextEditingController(text: '0x20000');

  String? _deviceVersion;

  /// Board model the head unit reports over BLE (null when it can't be read).
  String? _deviceModel;

  /// Board whose image will actually be flashed. Defaults to [_deviceModel]
  /// when it's known. Without device info the user must choose explicitly.
  String? _selectedModel;
  String? _bundled;

  /// Boards this APK can flash over USB with a full merged recovery image.
  Set<String> _mergedBoards = <String>{};

  /// Whether the head unit can be flashed over BLE (firmware exposes the OTA
  /// data characteristics). Drives whether the Bluetooth method is offered.
  bool _bleOtaSupported = false;

  /// Chosen update transport. Defaults to BLE when the head unit supports it
  /// (no WiFi juggling), else WiFi.
  OtaTransport _transport = OtaTransport.wifi;
  bool _loading = true;
  bool _busy = false;
  UpdateState _state = const UpdateState(UpdatePhase.idle);

  // --- USB state -------------------------------------------------------------
  bool get _usbSupported => Platform.isAndroid;
  bool _usbScanning = false;
  String? _usbScanError;
  List<UsbCandidate> _usbDevices = <UsbCandidate>[];
  UsbCandidate? _usbDevice;
  _UsbSource _usbSource = _UsbSource.bundled;

  /// Bundled USB flashing: false = full recovery image (bootloader+partitions),
  /// true = application partition only (keeps NVS/settings).
  bool _usbAppOnly = false;
  Uint8List? _customBytes;
  String? _customName;
  EspImageInfo? _customInfo;
  UsbFlashState _usbState = const UsbFlashState(UsbFlashPhase.idle);

  @override
  void initState() {
    super.initState();
    _updater.state.listen((s) {
      if (mounted) setState(() => _state = s);
    });
    _usb.state.listen((s) {
      if (mounted) setState(() => _usbState = s);
    });
    _load();
  }

  @override
  void dispose() {
    _usb.cancel();
    _usb.dispose();
    _updater.dispose();
    _hostCtrl.dispose();
    _offsetCtrl.dispose();
    super.dispose();
  }

  Future<void> _load() async {
    final bundled = await FirmwareUpdater.bundledVersion();
    // Best-effort: show the running version if BLE exposes it. Not required
    // for flashing.
    final info = BleProxy.instance.supportsOta
        ? await BleProxy.instance.readOtaInfo()
        : null;
    final merged = <String>{};
    for (final board in FirmwareUpdater.boards) {
      if (await FirmwareUpdater.hasMergedImage(board)) merged.add(board);
    }
    if (!mounted) return;
    final detected = info?.model;
    final bleOta = BleProxy.instance.supportsBleOta;
    setState(() {
      _bundled = bundled;
      _deviceVersion = info?.version;
      _deviceModel = detected;
      _selectedModel = FirmwareUpdater.boards.contains(detected)
          ? detected
          : null;
      _mergedBoards = merged;
      _bleOtaSupported = bleOta;
      _transport = bleOta ? OtaTransport.ble : OtaTransport.wifi;
      _loading = false;
    });
    if (_usbSupported) unawaited(_scanUsb());
  }

  // ------------------------------------------------------------------ USB ---

  Future<void> _scanUsb() async {
    if (!_usbSupported) return;
    setState(() {
      _usbScanning = true;
      _usbScanError = null;
    });
    try {
      final devices = await UsbDevices.scan();
      if (!mounted) return;
      setState(() {
        _usbDevices = devices;
        // Keep the selection when the same device is still attached.
        if (!devices.contains(_usbDevice)) {
          _usbDevice = devices.isNotEmpty ? devices.first : null;
        }
        _usbScanning = false;
      });
    } catch (e) {
      if (!mounted) return;
      setState(() {
        _usbDevices = <UsbCandidate>[];
        _usbDevice = null;
        _usbScanError = '$e';
        _usbScanning = false;
      });
    }
  }

  Future<void> _pickCustomImage() async {
    try {
      final picked = await FilePicker.platform.pickFiles();
      if (picked == null || picked.files.isEmpty) return;
      final file = picked.files.first;
      if (file.size > 0x2000000) {
        if (!mounted) return;
        setState(() {
          _customBytes = null;
          _customName = null;
          _customInfo = null;
          _usbScanError = t(context, 'fw.usb.err.toobig');
        });
        return;
      }
      Uint8List? bytes = file.bytes;
      if (bytes == null && file.path != null) {
        bytes = await File(file.path!).readAsBytes();
      }
      if (bytes == null || !mounted) return;
      final info = inspectEspImage(bytes);
      setState(() {
        _customBytes = bytes;
        _customName = file.name;
        _customInfo = info;
        if (info.isApplication) {
          _offsetCtrl.text = '0x${info.flashOffset.toRadixString(16)}';
        } else if (info.isMerged) {
          _offsetCtrl.text = '0x0';
        }
      });
    } catch (e) {
      if (!mounted) return;
      setState(() {
        _customBytes = null;
        _customName = null;
        _customInfo = null;
        _usbScanError = '$e';
      });
    }
  }

  int? _parseOffset() {
    final text = _offsetCtrl.text.trim();
    if (text.isEmpty) return null;
    final value = text.toLowerCase().startsWith('0x')
        ? int.tryParse(text.substring(2), radix: 16)
        : int.tryParse(text, radix: 16);
    if (value == null || value < 0 || value % 0x1000 != 0) return null;
    return value;
  }

  bool get _usbCanFlash {
    if (!_usbSupported || _busy || _usbDevice == null) return false;
    if (_usbSource == _UsbSource.bundled) {
      return FirmwareUpdater.boards.contains(_selectedModel);
    }
    final info = _customInfo;
    if (_customBytes == null || info == null || !info.isValid) return false;
    return info.isMerged || _parseOffset() != null;
  }

  Future<void> _confirmUsbFlash() async {
    final device = _usbDevice;
    if (device == null) return;
    Uint8List image;
    String name;
    int offset;
    if (_usbSource == _UsbSource.bundled) {
      final model = _selectedModel;
      if (!FirmwareUpdater.boards.contains(model)) return;
      final full = _mergedBoards.contains(model) && !_usbAppOnly;
      if (full) {
        final bundledImage = await FirmwareUpdater.usbImageFor(model);
        image = bundledImage.bytes;
        offset = 0;
        name = '${FirmwareUpdater.displayName(model)} '
            '${await FirmwareUpdater.bundledVersion()} (recovery)';
      } else {
        image = await FirmwareUpdater.imageFor(model);
        offset = 0x20000;
        name = '${FirmwareUpdater.displayName(model)} '
            '${await FirmwareUpdater.bundledVersion()} (app)';
      }
    } else {
      image = _customBytes!;
      name = _customName ?? 'firmware.bin';
      final info = _customInfo!;
      offset = info.isMerged ? 0 : (_parseOffset() ?? 0x20000);
    }
    if (!mounted) return;

    final lines = <String>[
      t(context, 'fw.usb.warn.body'),
      '',
      '${t(context, 'fw.usb.source')}: $name',
      '${t(context, 'fw.usb.port')}: ${device.label}',
      if (_usbSource == _UsbSource.bundled)
        '${t(context, 'fw.select')}: '
            '${FirmwareUpdater.displayName(_selectedModel)}',
      '${t(context, 'fw.usb.offset')}: 0x${offset.toRadixString(16)}',
    ];
    final go = await showDialog<bool>(
      context: context,
      builder: (ctx) => AlertDialog(
        title: Text(t(ctx, 'fw.warn.title')),
        content: Text(lines.join('\n')),
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
    if (go != true || !mounted) return;

    setState(() => _busy = true);
    await _usb.run(
      candidate: device,
      image: image,
      imageName: name,
      appOffset: offset,
    );
    if (mounted) {
      setState(() => _busy = false);
      unawaited(_scanUsb());
    }
  }

  // ------------------------------------------------------------------ OTA ---

  Future<void> _confirmAndFlash() async {
    if (!FirmwareUpdater.canFlash(_deviceModel, _selectedModel)) return;
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
    if (_selectedModel != _deviceModel) {
      return t(context, 'fw.warn.mismatch');
    }
    return null;
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: Text(t(context, 'fw.title')), actions: [
        IconButton(tooltip: 'Recovery', onPressed: _busy ? null : () => Navigator.push(context, MaterialPageRoute(builder: (_) => const RecoveryScreen())), icon: const Icon(Icons.health_and_safety_outlined)),
      ]),
      body: _loading
          ? const Center(child: CircularProgressIndicator())
          : SafeArea(
              child: Column(
                children: [
                  Expanded(
                    child: ListView(
                      padding: const EdgeInsets.all(16),
                      children: _body(context),
                    ),
                  ),
                  Padding(
                    padding: const EdgeInsets.fromLTRB(16, 4, 16, 16),
                    child: _actionButton(context),
                  ),
                ],
              ),
            ),
    );
  }

  Widget _actionButton(BuildContext context) {
    final usb = _transport == OtaTransport.usb;
    final enabled = usb
        ? _usbCanFlash
        : !_busy && FirmwareUpdater.canFlash(_deviceModel, _selectedModel);
    return Row(
      children: [
        if (usb && _usbState.busy)
          Padding(
            padding: const EdgeInsets.only(right: 8),
            child: OutlinedButton.icon(
              onPressed: () => _usb.cancel(),
              icon: const Icon(Icons.stop),
              label: Text(t(context, 'fw.usb.stop')),
            ),
          ),
        Expanded(
          child: FilledButton.icon(
            onPressed: enabled
                ? (usb ? _confirmUsbFlash : _confirmAndFlash)
                : null,
            icon: _busy
                ? const SizedBox(
                    width: 18,
                    height: 18,
                    child: CircularProgressIndicator(strokeWidth: 2))
                : Icon(usb ? Icons.usb : Icons.system_update),
            label: Text(_busy
                ? t(context, 'fw.flashing')
                : t(context, usb ? 'fw.usb.flash' : 'fw.flash')),
          ),
        ),
      ],
    );
  }

  List<Widget> _body(BuildContext context) {
    final bundled = _bundled ?? '?';
    final upToDate = _deviceVersion != null && _deviceVersion == bundled;
    final usb = _transport == OtaTransport.usb;

    return [
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
      Align(
        alignment: Alignment.centerLeft,
        child: Text(t(context, 'fw.method'),
            style: Theme.of(context).textTheme.labelLarge),
      ),
      const SizedBox(height: 6),
      SegmentedButton<OtaTransport>(
        segments: [
          if (_usbSupported)
            ButtonSegment(
              value: OtaTransport.usb,
              icon: const Icon(Icons.usb),
              label: Text(t(context, 'fw.method.usb')),
            ),
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
        onSelectionChanged: _busy
            ? null
            : (s) {
                setState(() => _transport = s.first);
                if (s.first == OtaTransport.usb && _usbDevices.isEmpty) {
                  unawaited(_scanUsb());
                }
              },
      ),
      const SizedBox(height: 6),
      Row(
        children: [
          Icon(Icons.info_outline, size: 16, color: Colors.grey[600]),
          const SizedBox(width: 6),
          Expanded(
            child: Text(
                usb
                    ? t(context, 'fw.method.usb.note')
                    : (_transport == OtaTransport.ble
                        ? t(context, 'fw.method.ble.note')
                        : (_bleOtaSupported
                            ? t(context, 'fw.host.note')
                            : t(context, 'fw.method.ble.unsupported'))),
                style: TextStyle(color: Colors.grey[600], fontSize: 13)),
          ),
        ],
      ),
      if (usb) ..._usbBody(context) else ..._otaBody(context, upToDate),
      ..._progress(context),
    ];
  }

  // ------------------------------------------------------------- USB UI ---

  List<Widget> _usbBody(BuildContext context) {
    final sourceBundled = _usbSource == _UsbSource.bundled;
    final info = _customInfo;
    final model = _selectedModel;
    final merged = model != null && _mergedBoards.contains(model);
    return [
      const SizedBox(height: 16),
      Align(
        alignment: Alignment.centerLeft,
        child: Text(t(context, 'fw.usb.source'),
            style: Theme.of(context).textTheme.labelLarge),
      ),
      const SizedBox(height: 6),
      SegmentedButton<_UsbSource>(
        segments: [
          ButtonSegment(
            value: _UsbSource.bundled,
            icon: const Icon(Icons.inventory_2_outlined),
            label: Text(t(context, 'fw.usb.source.bundled')),
          ),
          ButtonSegment(
            value: _UsbSource.file,
            icon: const Icon(Icons.folder_open),
            label: Text(t(context, 'fw.usb.source.file')),
          ),
        ],
        selected: {_usbSource},
        onSelectionChanged:
            _busy ? null : (s) => setState(() => _usbSource = s.first),
      ),
      const SizedBox(height: 12),
      if (sourceBundled) ...[
        DropdownButtonFormField<String>(
          value: model,
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
          onChanged: _busy ? null : (v) => setState(() => _selectedModel = v),
        ),
        if (model == null) ...[
          const SizedBox(height: 6),
          _hint(context, t(context, 'fw.usb.board.hint'), Colors.orange),
        ] else if (_modelWarning(context) case final w?) ...[
          const SizedBox(height: 6),
          _hint(context, w, Colors.orange),
        ],
        const SizedBox(height: 8),
        if (merged)
          SwitchListTile(
            value: _usbAppOnly,
            onChanged: _busy ? null : (v) => setState(() => _usbAppOnly = v),
            contentPadding: EdgeInsets.zero,
            title: Text(t(context, 'fw.usb.apponly')),
            subtitle: Text(t(context, 'fw.usb.apponly.keep')),
          ),
        _hint(
          context,
          !merged
              ? t(context, 'fw.usb.apponly.note')
              : (_usbAppOnly
                  ? t(context, 'fw.usb.apponly.chosen.note')
                  : t(context, 'fw.usb.merged.note')),
          merged && !_usbAppOnly ? Colors.grey[600] : Colors.orange,
          icon: merged && !_usbAppOnly
              ? Icons.verified_outlined
              : Icons.warning_amber,
        ),
      ] else ...[
        OutlinedButton.icon(
          onPressed: _busy ? null : _pickCustomImage,
          icon: const Icon(Icons.folder_open),
          label: Text(t(context, 'fw.usb.pick')),
        ),
        const SizedBox(height: 8),
        if (_customBytes == null)
          _hint(context, t(context, 'fw.usb.file.none'), Colors.grey[600])
        else ...[
          _hint(
            context,
            '${_customName ?? 'firmware.bin'} · '
            '${(_customBytes!.length / 1024).round()} KB',
            Colors.grey[700],
            icon: Icons.description_outlined,
          ),
          const SizedBox(height: 4),
          _hint(
            context,
            info == null
                ? t(context, 'fw.usb.file.none')
                : (info.isValid
                    ? t(context,
                        info.isMerged ? 'fw.usb.file.merged' : 'fw.usb.file.app')
                    : t(context, info.errorKey ?? 'fw.usb.file.invalid')),
            info != null && info.isValid
                ? (info.isMerged ? Colors.green : Colors.orange)
                : Colors.red,
            icon: info != null && info.isValid
                ? Icons.check_circle_outline
                : Icons.error_outline,
          ),
          if (info != null && info.isValid && info.isApplication) ...[
            const SizedBox(height: 12),
            TextField(
              controller: _offsetCtrl,
              enabled: !_busy,
              decoration: InputDecoration(
                labelText: t(context, 'fw.usb.offset'),
                border: const OutlineInputBorder(),
                prefixIcon: const Icon(Icons.tag),
                hintText: '0x20000',
              ),
              onChanged: (_) => setState(() {}),
            ),
            if (_parseOffset() == null) ...[
              const SizedBox(height: 4),
              _hint(context, t(context, 'fw.usb.err.offset'), Colors.orange),
            ],
          ],
          if (info != null && info.isMerged) ...[
            const SizedBox(height: 6),
            _hint(context, t(context, 'fw.usb.custom.merged.warn'),
                Colors.orange,
                icon: Icons.warning_amber),
          ],
        ],
      ],
      const SizedBox(height: 16),
      Row(
        children: [
          Expanded(
            child: DropdownButtonFormField<UsbCandidate>(
              value: _usbDevice,
              isExpanded: true,
              decoration: InputDecoration(
                labelText: t(context, 'fw.usb.port'),
                border: const OutlineInputBorder(),
                prefixIcon: const Icon(Icons.usb),
              ),
              items: [
                for (final d in _usbDevices)
                  DropdownMenuItem(
                    value: d,
                    child: Text(d.label, overflow: TextOverflow.ellipsis),
                  ),
              ],
              onChanged: (_busy || _usbScanning)
                  ? null
                  : (v) => setState(() => _usbDevice = v),
            ),
          ),
          const SizedBox(width: 8),
          IconButton(
            tooltip: t(context, 'fw.usb.scan'),
            onPressed: (_busy || _usbScanning) ? null : _scanUsb,
            icon: _usbScanning
                ? const SizedBox(
                    width: 20,
                    height: 20,
                    child: CircularProgressIndicator(strokeWidth: 2))
                : const Icon(Icons.refresh),
          ),
        ],
      ),
      if (_usbDevices.isEmpty && !_usbScanning) ...[
        const SizedBox(height: 6),
        _hint(context, t(context, 'fw.usb.none'), Colors.orange,
            icon: Icons.usb_off),
      ],
      if (_usbScanError != null) ...[
        const SizedBox(height: 6),
        _hint(context, _usbScanError!, Colors.red, icon: Icons.error_outline),
      ],
    ];
  }

  // ------------------------------------------------------------- OTA UI ---

  List<Widget> _otaBody(BuildContext context, bool upToDate) {
    return [
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
        onChanged: _busy ? null : (v) => setState(() => _selectedModel = v),
      ),
      if (_modelWarning(context) case final w?) ...[
        const SizedBox(height: 6),
        _hint(context, w, Colors.orange, icon: Icons.warning_amber),
      ],
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
    ];
  }

  // ------------------------------------------------------------ progress ---

  List<Widget> _progress(BuildContext context) {
    if (_transport == OtaTransport.usb) {
      final s = _usbState;
      if (s.phase == UsbFlashPhase.idle) return const [SizedBox(height: 12)];
      final showBar = s.phase == UsbFlashPhase.writing;
      final isError = s.phase == UsbFlashPhase.error;
      final isDone = s.phase == UsbFlashPhase.done;
      return [
        const SizedBox(height: 12),
        if (showBar) ...[
          LinearProgressIndicator(value: s.progress),
          const SizedBox(height: 4),
          Text('${(s.progress * 100).round()}%',
              textAlign: TextAlign.center),
          const SizedBox(height: 8),
        ],
        if (_localizeUsb(context, s) case final msg?)
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
      ];
    }

    final s = _state;
    if (s.phase == UpdatePhase.idle) return const [SizedBox(height: 12)];
    final showBar = s.phase == UpdatePhase.uploading;
    final isError = s.phase == UpdatePhase.error;
    final isDone = s.phase == UpdatePhase.done;
    return [
      const SizedBox(height: 12),
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
    ];
  }

  Widget _hint(BuildContext context, String text, Color? color,
      {IconData icon = Icons.info_outline}) {
    return Row(
      children: [
        Icon(icon, size: 16, color: color),
        const SizedBox(width: 6),
        Expanded(
          child: Text(text,
              style: TextStyle(color: color, fontSize: 13)),
        ),
      ],
    );
  }

  String? _localizeUsb(BuildContext context, UsbFlashState s) {
    final key = s.messageKey;
    if (key == null) return null;
    var text = t(context, key);
    s.args?.forEach((k, v) => text = text.replaceAll('{$k}', v));
    return text;
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
