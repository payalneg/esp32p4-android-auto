/// LISP editor over the head unit's NUS bridge — a phone-side mini VESC Tool
/// "VESC Scripting" page. Two tabs:
///   Code      — edit the script; open/save local files, read/upload/run/stop
///               on the VESC.
///   Variables — live LISP runtime bindings (cpu/heap/mem/stack + globals).
///
/// The VESC protocol lives in the background isolate (see lib/ble/vesc/); this
/// screen only drives it through [LispProxy].
library;

import 'dart:convert';
import 'dart:io';
import 'dart:typed_data';

import 'package:file_picker/file_picker.dart';
import 'package:flutter/material.dart';

import '../ble/ble_proxy.dart';
import '../ble/lisp_models.dart';
import '../i18n/strings.dart';

class LispEditorScreen extends StatefulWidget {
  const LispEditorScreen({super.key});

  @override
  State<LispEditorScreen> createState() => _LispEditorScreenState();
}

class _LispEditorScreenState extends State<LispEditorScreen>
    with SingleTickerProviderStateMixin {
  final _code = TextEditingController();
  final _lisp = LispProxy.instance;
  late final TabController _tab;

  bool _busy = false;
  double? _progress;
  bool _statsOn = false;

  @override
  void initState() {
    super.initState();
    _tab = TabController(length: 2, vsync: this);
    _tab.addListener(_syncStats);
  }

  @override
  void dispose() {
    _tab.removeListener(_syncStats);
    if (_statsOn) _lisp.stopStats();
    _tab.dispose();
    _code.dispose();
    super.dispose();
  }

  /// Poll stats only while the Variables tab is showing.
  void _syncStats() {
    final want = _tab.index == 1;
    if (want == _statsOn) return;
    _statsOn = want;
    if (want) {
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

  // ---- device (VESC over NUS) ----

  Future<void> _read() => _guard(() async {
        final code = await _lisp.read(onProgress: _onProgress);
        _code.text = code;
      }, okKey: 'lisp.loaded');

  Future<void> _upload({required bool run}) => _guard(() async {
        await _lisp.upload(_code.text, run: run, onProgress: _onProgress);
      }, okKey: run ? 'lisp.uploadedRun' : 'lisp.uploaded');

  Future<void> _run() => _guard(() => _lisp.run());
  Future<void> _stop() => _guard(() => _lisp.stop());

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
        ],
        bottom: TabBar(
          controller: _tab,
          tabs: [
            Tab(text: t(context, 'lisp.tab.code')),
            Tab(text: t(context, 'lisp.tab.vars')),
          ],
        ),
      ),
      body: Column(
        children: [
          if (_progress != null)
            LinearProgressIndicator(value: _progress)
          else if (_busy)
            const LinearProgressIndicator(),
          Expanded(
            child: TabBarView(
              controller: _tab,
              children: [
                _codeTab(context),
                _varsTab(context),
              ],
            ),
          ),
        ],
      ),
    );
  }

  Widget _codeTab(BuildContext context) {
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
                Text(t(context, 'lisp.gaugesPaused'),
                    style: Theme.of(context).textTheme.bodySmall),
                const SizedBox(height: 6),
                Wrap(
                  spacing: 8,
                  runSpacing: 4,
                  alignment: WrapAlignment.center,
                  children: [
                    OutlinedButton.icon(
                      icon: const Icon(Icons.download),
                      label: Text(t(context, 'lisp.read')),
                      onPressed: _busy ? null : _read,
                    ),
                    OutlinedButton.icon(
                      icon: const Icon(Icons.upload),
                      label: Text(t(context, 'lisp.upload')),
                      onPressed: _busy ? null : () => _upload(run: false),
                    ),
                    FilledButton.icon(
                      icon: const Icon(Icons.play_circle),
                      label: Text(t(context, 'lisp.uploadRun')),
                      onPressed: _busy ? null : () => _upload(run: true),
                    ),
                    OutlinedButton.icon(
                      icon: const Icon(Icons.play_arrow),
                      label: Text(t(context, 'lisp.run')),
                      onPressed: _busy ? null : _run,
                    ),
                    OutlinedButton.icon(
                      icon: const Icon(Icons.stop),
                      label: Text(t(context, 'lisp.stop')),
                      onPressed: _busy ? null : _stop,
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
