/// LISP editor — a phone-side mini VESC Tool "VESC Scripting" page. Two tabs:
///   Code      — edit the script; open/save local files, check it, and
///               read/upload/run/stop on the VESC.
///   Assistant — an AI agent that edits the SAME buffer, flashes it once you
///               confirm, and verifies on the hardware that it runs.
///
/// Live device state (runtime variables and the script's `(print ...)` output)
/// belongs to neither tab, so it lives in a bottom sheet reachable from both.
///
/// The VESC is reached over a Nordic UART Service link, picked at the top of
/// the screen: either the head unit's built-in bridge, or any stand-alone VESC
/// BLE adapter — the latter works with no head unit at all. A direct adapter is
/// connected while this screen is open and released when it closes.
///
/// The VESC protocol lives in the background isolate (see lib/ble/vesc/); this
/// screen only drives it through [LispProxy].
library;

import 'dart:async';
import 'dart:convert';
import 'dart:io';
import 'dart:typed_data';

import 'package:file_picker/file_picker.dart';
import 'package:flutter/material.dart';
import 'package:permission_handler/permission_handler.dart';

import '../agent/agent_session.dart';
import '../agent/lisp_device.dart';
import '../agent/lisp_lint.dart';
import '../ble/ble_proxy.dart';
import '../ble/lisp_models.dart';
import '../bridge/foreground_bridge.dart';
import '../i18n/strings.dart';
import 'agent_tab.dart';
import 'console_panel.dart';
import 'lisp_syntax.dart';

class LispEditorScreen extends StatefulWidget {
  const LispEditorScreen({super.key});

  @override
  State<LispEditorScreen> createState() => _LispEditorScreenState();
}

class _LispEditorScreenState extends State<LispEditorScreen>
    with SingleTickerProviderStateMixin {
  /// Colours itself as you type — see lisp_syntax.dart. The agent edits this
  /// very controller, so its patches land highlighted too.
  final _code = LispEditingController();
  final _lisp = LispProxy.instance;
  late final TabController _tab;
  late final AgentSession _agent;
  StreamSubscription<VescTargetInfo>? _targetSub;
  AppLifecycleListener? _lifecycle;

  bool _busy = false;
  double? _progress;
  bool _statsOn = false;
  VescTargetInfo _target = LispProxy.instance.target;

  @override
  void initState() {
    super.initState();
    _tab = TabController(length: 2, vsync: this);
    _agent = AgentSession(device: LispProxyDevice(), code: _code);
    // The agent must not flash the VESC while nobody is watching the screen.
    _lifecycle = AppLifecycleListener(
      onStateChange: (s) => _agent.setBackgrounded(
          s != AppLifecycleState.resumed && s != AppLifecycleState.inactive),
    );
    // Bring back the previous conversation — a session costs money and flash
    // cycles, so leaving the screen must not throw it away.
    unawaited(_agent.restore());
    _targetSub = _lisp.targets.listen((t) {
      if (mounted) setState(() => _target = t);
    });
    // Bring the persisted target up (reconnects a saved adapter).
    unawaited(_lisp.resume().catchError((_) {}));
  }

  @override
  void dispose() {
    _lifecycle?.dispose();
    unawaited(_agent.dispose());
    if (_statsOn) _lisp.stopStats();
    // Let go of a direct adapter link — it's only held while this screen lives.
    unawaited(_lisp.release().catchError((_) {}));
    _targetSub?.cancel();
    _tab.dispose();
    _code.dispose();
    super.dispose();
  }

  /// Runtime variables and console output are device state, not tab state, so
  /// they live in a sheet reachable from both tabs. Stats polling costs a VESC
  /// round-trip 2.5 times a second, so it runs only while that sheet is open.
  void _setStats(bool on) {
    if (on == _statsOn) return;
    _statsOn = on;
    if (on) {
      _lisp.startStats();
    } else {
      _lisp.stopStats();
    }
  }

  void _snack(String msg) {
    if (!mounted) return;
    ScaffoldMessenger.of(context)
        .showSnackBar(SnackBar(content: Text(msg)));
  }

  void _showError(Object e) {
    if (!mounted) return;
    final msg = e is LispException ? t(context, e.key) : '$e';
    _snack(msg);
  }

  Future<void> _guard(Future<void> Function() op, {String? okKey}) async {
    if (_busy) return;
    setState(() {
      _busy = true;
      _progress = null;
    });
    try {
      await op();
      if (okKey != null && mounted) _snack(t(context, okKey));
    } catch (e) {
      _showError(e);
    } finally {
      if (mounted) {
        setState(() {
          _busy = false;
          _progress = null;
        });
      }
    }
  }

  void _onProgress(double f) {
    if (mounted) setState(() => _progress = f);
  }

  /// Structural check of the buffer. Every rule it enforces is one the VESC
  /// accepts silently and then fails on at runtime, so this runs offline and
  /// before any flash.
  void _lint() {
    final r = lintLisp(_code.text);
    showDialog<void>(
      context: context,
      builder: (ctx) => AlertDialog(
        title: Text(t(ctx, 'lisp.lint.title')),
        content: SizedBox(
          width: double.maxFinite,
          child: Column(
            mainAxisSize: MainAxisSize.min,
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              Text(
                t(ctx, 'lisp.lint.summary')
                    .replaceAll('{e}', '${r.errors.length}')
                    .replaceAll('{w}', '${r.warnings.length}')
                    .replaceAll(
                        '{kb}', (r.packedBytes / 1024).toStringAsFixed(1)),
                style: Theme.of(ctx).textTheme.bodySmall,
              ),
              const SizedBox(height: 8),
              if (r.issues.isEmpty)
                Text(t(ctx, 'lisp.lint.clean'))
              else
                Flexible(
                  child: ListView(
                    shrinkWrap: true,
                    children: [
                      for (final i in r.issues)
                        ListTile(
                          dense: true,
                          contentPadding: EdgeInsets.zero,
                          leading: Icon(
                            switch (i.level) {
                              LintLevel.error => Icons.error_outline,
                              LintLevel.warn => Icons.warning_amber,
                              LintLevel.info => Icons.info_outline,
                            },
                            color: switch (i.level) {
                              LintLevel.error =>
                                Theme.of(ctx).colorScheme.error,
                              LintLevel.warn => Colors.orange,
                              LintLevel.info =>
                                Theme.of(ctx).colorScheme.outline,
                            },
                          ),
                          title: Text('${i.line}: ${i.message}',
                              style: Theme.of(ctx).textTheme.bodySmall),
                          subtitle: i.hint == null
                              ? null
                              : Text(i.hint!,
                                  style: Theme.of(ctx)
                                      .textTheme
                                      .bodySmall
                                      ?.copyWith(
                                          color: Theme.of(ctx)
                                              .colorScheme
                                              .outline)),
                        ),
                    ],
                  ),
                ),
            ],
          ),
        ),
        actions: [
          TextButton(
            onPressed: () => Navigator.of(ctx).pop(),
            child: Text(t(ctx, 'lisp.lint.close')),
          ),
        ],
      ),
    );
  }

  /// Live device state — runtime variables and the script's `(print ...)`
  /// output — as a draggable sheet shared by both tabs.
  Future<void> _openDeviceSheet({int initialTab = 0}) async {
    _setStats(true);
    await showModalBottomSheet<void>(
      context: context,
      isScrollControlled: true,
      showDragHandle: true,
      builder: (_) => DraggableScrollableSheet(
        expand: false,
        initialChildSize: 0.65,
        minChildSize: 0.3,
        maxChildSize: 0.95,
        builder: (_, __) => DefaultTabController(
          length: 2,
          initialIndex: initialTab,
          child: Column(
            children: [
              TabBar(tabs: [
                Tab(text: t(context, 'lisp.tab.vars')),
                Tab(text: t(context, 'lisp.console.title')),
              ]),
              Expanded(
                child: TabBarView(children: [
                  _varsTab(context),
                  const ConsolePanel(),
                ]),
              ),
            ],
          ),
        ),
      ),
    );
    _setStats(false);
  }

  /// "12.3 KB in 4.2 s - 2.9 KB/s" — how long the transfer actually took, so
  /// the head-unit bridge and a direct adapter can be compared on hardware.
  String _rate(VescXferStats? s) {
    if (s == null || s.ms <= 0) return '';
    return ' · ${(s.bytes / 1024).toStringAsFixed(1)} KB / '
        '${(s.ms / 1000).toStringAsFixed(1)} s · '
        '${s.kbs.toStringAsFixed(1)} KB/s';
  }

  // ---- device (VESC over NUS) ----

  Future<void> _read() => _guard(() async {
        final r = await _lisp.read(onProgress: _onProgress);
        _code.text = r.code;
        if (mounted) _snack('${t(context, 'lisp.loaded')}${_rate(r.stats)}');
      });

  Future<void> _upload({required bool run}) => _guard(() async {
        final s = await _lisp.upload(_code.text, run: run, onProgress: _onProgress);
        if (mounted) {
          _snack(
              '${t(context, run ? 'lisp.uploadedRun' : 'lisp.uploaded')}${_rate(s)}');
        }
      });

  Future<void> _run() => _guard(() => _lisp.run());
  Future<void> _stop() => _guard(() => _lisp.stop());

  // ---- adapter picker ----

  String _targetTitle(BuildContext context) {
    if (_target.kind == VescTargetKind.headUnit) {
      return t(context, 'lisp.adapter.headunit');
    }
    final n = _target.name;
    return (n == null || n.isEmpty)
        ? (_target.remoteId ?? t(context, 'lisp.adapter.none'))
        : n;
  }

  String _targetState(BuildContext context) => switch (_target.state) {
        VescLinkState.idle => _target.kind == VescTargetKind.headUnit
            ? t(context, 'lisp.adapter.headunit.off')
            : t(context, 'lisp.state.idle'),
        VescLinkState.connecting => t(context, 'lisp.state.connecting'),
        VescLinkState.connected => t(context, 'lisp.state.connected'),
        VescLinkState.failed => t(context, 'lisp.state.failed'),
      };

  Future<void> _pickAdapter() async {
    var devices = <ScanDevice>[];
    var scanning = false;
    var showAll = false;
    var connecting = false;

    await showModalBottomSheet<void>(
      context: context,
      isScrollControlled: true,
      builder: (sheetCtx) => StatefulBuilder(
        builder: (sheetCtx, setSheet) {
          Future<void> scan() async {
            setSheet(() => scanning = true);
            try {
              // Same preamble as the pairing screen: without BLUETOOTH_SCAN /
              // a running foreground service the background isolate can't scan.
              await [
                Permission.bluetoothScan,
                Permission.bluetoothConnect,
                Permission.locationWhenInUse,
              ].request();
              if (Platform.isAndroid &&
                  await Permission.bluetoothConnect.isGranted) {
                await ForegroundBridge().start();
              }
              final r = await BleProxy.instance.scan(quiet: true);
              r.sort((a, b) {
                if (a.nus != b.nus) return a.nus ? -1 : 1;
                return b.rssi.compareTo(a.rssi);
              });
              setSheet(() => devices = r);
            } catch (e) {
              _showError(e);
            } finally {
              setSheet(() => scanning = false);
            }
          }

          Future<void> pick(Future<void> Function() select) async {
            setSheet(() => connecting = true);
            try {
              await select();
              if (sheetCtx.mounted) Navigator.pop(sheetCtx);
            } catch (e) {
              _showError(e);
            } finally {
              if (sheetCtx.mounted) setSheet(() => connecting = false);
            }
          }

          final shown =
              showAll ? devices : devices.where((d) => d.nus).toList();
          return SafeArea(
            child: Column(
              mainAxisSize: MainAxisSize.min,
              children: [
                ListTile(
                  title: Text(t(context, 'lisp.adapter.pick'),
                      style: Theme.of(context).textTheme.titleLarge),
                  trailing: IconButton(
                    icon: const Icon(Icons.refresh),
                    tooltip: t(context, 'lisp.adapter.scan'),
                    onPressed: scanning || connecting ? null : scan,
                  ),
                ),
                if (scanning || connecting) const LinearProgressIndicator(),
                ListTile(
                  leading: const Icon(Icons.tv),
                  title: Text(t(context, 'lisp.adapter.headunit')),
                  subtitle: Text(_target.headUnitAvailable
                      ? t(context, 'lisp.adapter.headunit.on')
                      : t(context, 'lisp.adapter.headunit.off')),
                  enabled: _target.headUnitAvailable && !connecting,
                  selected: _target.kind == VescTargetKind.headUnit,
                  onTap: () => pick(_lisp.selectHeadUnit),
                ),
                const Divider(height: 1),
                Flexible(
                  child: shown.isEmpty
                      ? Padding(
                          padding: const EdgeInsets.all(24),
                          child: Text(
                            scanning
                                ? t(context, 'lisp.adapter.scanning')
                                : t(context, 'lisp.adapter.empty'),
                            textAlign: TextAlign.center,
                          ),
                        )
                      : ListView.builder(
                          shrinkWrap: true,
                          itemCount: shown.length,
                          itemBuilder: (_, i) {
                            final d = shown[i];
                            return ListTile(
                              leading: Icon(d.nus
                                  ? Icons.settings_input_antenna
                                  : Icons.bluetooth),
                              title: Text(d.name.isNotEmpty
                                  ? d.name
                                  : t(context, 'pairing.unnamed')),
                              subtitle:
                                  Text('${d.remoteId} · rssi ${d.rssi} dBm'),
                              selected: _target.kind == VescTargetKind.direct &&
                                  _target.remoteId == d.remoteId,
                              enabled: !connecting,
                              onTap: () => pick(
                                  () => _lisp.selectDirect(d.remoteId, d.name)),
                            );
                          },
                        ),
                ),
                SwitchListTile(
                  dense: true,
                  value: showAll,
                  title: Text(t(context, 'lisp.adapter.showAll')),
                  subtitle: Text(t(context, 'lisp.adapter.nusHint')),
                  onChanged: (v) => setSheet(() => showAll = v),
                ),
              ],
            ),
          );
        },
      ),
    );
  }

  Widget _adapterBar(BuildContext context) {
    final ok = _target.connected;
    return Material(
      color: Theme.of(context).colorScheme.surfaceContainerHigh,
      child: ListTile(
        dense: true,
        leading: Icon(
          _target.kind == VescTargetKind.headUnit
              ? Icons.tv
              : Icons.settings_input_antenna,
          color: ok ? Colors.green : Colors.orange,
        ),
        title: Text(_targetTitle(context)),
        subtitle: Text(_targetState(context)),
        trailing: TextButton(
          onPressed: _busy ? null : _pickAdapter,
          child: Text(t(context, 'lisp.adapter.change')),
        ),
      ),
    );
  }

  // ---- local files (phone) ----

  Future<void> _open() async {
    if (_busy) return;
    try {
      final res = await FilePicker.platform.pickFiles(withData: true);
      if (res == null || res.files.isEmpty) return;
      final f = res.files.single;
      String text;
      if (f.bytes != null) {
        text = utf8.decode(f.bytes!, allowMalformed: true);
      } else if (f.path != null) {
        text = await File(f.path!).readAsString();
      } else {
        return;
      }
      if (mounted) {
        setState(() => _code.text = text);
        _snack(t(context, 'lisp.opened'));
      }
    } catch (e) {
      _showError(e);
    }
  }

  Future<void> _save() async {
    try {
      final bytes = Uint8List.fromList(utf8.encode(_code.text));
      final path = await FilePicker.platform.saveFile(
        dialogTitle: t(context, 'lisp.save'),
        fileName: 'script.lisp',
        bytes: bytes,
      );
      if (path == null) return;
      // On desktop saveFile only returns a path; on mobile it already wrote
      // the bytes. Writing again is a harmless no-op on mobile (guarded).
      try {
        await File(path).writeAsBytes(bytes);
      } catch (_) {}
      if (mounted) _snack(t(context, 'lisp.saved'));
    } catch (e) {
      _showError(e);
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: Text(t(context, 'lisp.editor.title')),
        actions: [
          IconButton(
            icon: const Icon(Icons.folder_open),
            tooltip: t(context, 'lisp.open'),
            onPressed: _busy ? null : _open,
          ),
          IconButton(
            icon: const Icon(Icons.save),
            tooltip: t(context, 'lisp.save'),
            onPressed: _busy ? null : _save,
          ),
          IconButton(
            icon: const Icon(Icons.monitor_heart_outlined),
            tooltip: t(context, 'lisp.tab.vars'),
            onPressed: () => _openDeviceSheet(),
          ),
          IconButton(
            icon: const Icon(Icons.terminal),
            tooltip: t(context, 'lisp.console.title'),
            onPressed: () => _openDeviceSheet(initialTab: 1),
          ),
        ],
        bottom: TabBar(
          controller: _tab,
          tabs: [
            Tab(text: t(context, 'lisp.tab.code')),
            Tab(text: t(context, 'agent.tab')),
          ],
        ),
      ),
      body: Column(
        children: [
          _adapterBar(context),
          if (_progress != null)
            LinearProgressIndicator(value: _progress)
          else if (_busy)
            const LinearProgressIndicator(),
          Expanded(
            child: TabBarView(
              controller: _tab,
              children: [
                _codeTab(context),
                AgentTab(session: _agent),
              ],
            ),
          ),
        ],
      ),
    );
  }

  Widget _codeTab(BuildContext context) {
    // Device actions need a live link; file open/save and editing don't.
    final linked = _target.connected;
    return Column(
      children: [
        Expanded(
          child: Padding(
            padding: const EdgeInsets.all(8),
            child: TextField(
              controller: _code,
              maxLines: null,
              expands: true,
              textAlignVertical: TextAlignVertical.top,
              keyboardType: TextInputType.multiline,
              style: const TextStyle(fontFamily: 'monospace', fontSize: 13),
              decoration: InputDecoration(
                border: const OutlineInputBorder(),
                hintText: t(context, 'lisp.editor.hint'),
              ),
            ),
          ),
        ),
        SafeArea(
          top: false,
          child: Padding(
            padding: const EdgeInsets.fromLTRB(8, 0, 8, 8),
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.stretch,
              children: [
                Text(
                    linked
                        ? t(context, 'lisp.gaugesPaused')
                        : t(context, 'lisp.err.notarget'),
                    style: Theme.of(context).textTheme.bodySmall),
                const SizedBox(height: 6),
                Wrap(
                  spacing: 8,
                  runSpacing: 4,
                  alignment: WrapAlignment.center,
                  children: [
                    // Checking needs no link — it's the offline gate that
                    // catches the failures the VESC reports as success.
                    OutlinedButton.icon(
                      icon: const Icon(Icons.rule),
                      label: Text(t(context, 'lisp.lint.check')),
                      onPressed: _busy ? null : _lint,
                    ),
                    OutlinedButton.icon(
                      icon: const Icon(Icons.download),
                      label: Text(t(context, 'lisp.read')),
                      onPressed: _busy || !linked ? null : _read,
                    ),
                    OutlinedButton.icon(
                      icon: const Icon(Icons.upload),
                      label: Text(t(context, 'lisp.upload')),
                      onPressed:
                          _busy || !linked ? null : () => _upload(run: false),
                    ),
                    FilledButton.icon(
                      icon: const Icon(Icons.play_circle),
                      label: Text(t(context, 'lisp.uploadRun')),
                      onPressed:
                          _busy || !linked ? null : () => _upload(run: true),
                    ),
                    OutlinedButton.icon(
                      icon: const Icon(Icons.play_arrow),
                      label: Text(t(context, 'lisp.run')),
                      onPressed: _busy || !linked ? null : _run,
                    ),
                    OutlinedButton.icon(
                      icon: const Icon(Icons.stop),
                      label: Text(t(context, 'lisp.stop')),
                      onPressed: _busy || !linked ? null : _stop,
                    ),
                  ],
                ),
              ],
            ),
          ),
        ),
      ],
    );
  }

  Widget _varsTab(BuildContext context) {
    return StreamBuilder<LispStats>(
      stream: _lisp.stats,
      builder: (ctx, snap) {
        final s = snap.data;
        if (s == null) {
          return Center(child: Text(t(context, 'lisp.vars.waiting')));
        }
        return Column(
          crossAxisAlignment: CrossAxisAlignment.stretch,
          children: [
            Padding(
              padding: const EdgeInsets.all(12),
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  Text(
                    'CPU ${s.cpu.toStringAsFixed(1)}%   '
                    'Heap ${s.heap.toStringAsFixed(1)}%   '
                    'Mem ${s.mem.toStringAsFixed(1)}%   '
                    'Stack ${s.stack.toStringAsFixed(1)}%',
                    style: Theme.of(context).textTheme.bodySmall,
                  ),
                  if (s.doneCtx.isNotEmpty)
                    Padding(
                      padding: const EdgeInsets.only(top: 4),
                      child: Text(s.doneCtx,
                          style: Theme.of(context).textTheme.bodySmall),
                    ),
                ],
              ),
            ),
            const Divider(height: 1),
            if (s.bindings.isEmpty)
              Expanded(
                child: Center(child: Text(t(context, 'lisp.vars.none'))),
              )
            else
              Expanded(
                child: ListView.separated(
                  itemCount: s.bindings.length,
                  separatorBuilder: (_, __) => const Divider(height: 1),
                  itemBuilder: (ctx, i) {
                    final b = s.bindings[i];
                    return ListTile(
                      dense: true,
                      title: Text(b.name,
                          style:
                              const TextStyle(fontFamily: 'monospace')),
                      trailing: Text(
                        _fmt(b.value),
                        style: const TextStyle(fontFamily: 'monospace'),
                      ),
                    );
                  },
                ),
              ),
          ],
        );
      },
    );
  }

  String _fmt(double v) {
    if (v == v.roundToDouble() && v.abs() < 1e15) {
      return v.toInt().toString();
    }
    return v.toStringAsFixed(4);
  }
}
