/// Live view of the VESC's asynchronous output — everything the running LISP
/// script writes with `(print ...)`.
///
/// The bytes have always reached the phone (the head unit's bridge forwards
/// every packet the VESC sends), they were simply dropped by the link layer
/// for want of a matching request. [LispProxy.console] is that tap.
///
/// The panel catches up via [LispProxy.readConsole] on open — the ring in the
/// background isolate keeps buffering whether anyone is watching or not — then
/// follows the live stream. Live push is only enabled while the panel is
/// mounted, so a closed panel costs nothing on the port.
library;

import 'dart:async';

import 'package:flutter/material.dart';
import 'package:flutter/services.dart';

import '../ble/ble_proxy.dart';
import '../ble/lisp_models.dart';
import '../i18n/strings.dart';

class ConsolePanel extends StatefulWidget {
  const ConsolePanel({super.key});

  @override
  State<ConsolePanel> createState() => _ConsolePanelState();
}

class _ConsolePanelState extends State<ConsolePanel> {
  final _lisp = LispProxy.instance;
  final _lines = <LispConsoleLine>[];
  final _scroll = ScrollController();
  StreamSubscription<LispConsoleLine>? _sub;
  int _dropped = 0;
  bool _alive = false;
  bool _raw = false;
  bool _pinned = true;

  @override
  void initState() {
    super.initState();
    _scroll.addListener(_trackPin);
    unawaited(_start());
  }

  Future<void> _start() async {
    try {
      final chunk = await _lisp.readConsole(maxLines: 300);
      if (!mounted) return;
      setState(() {
        _lines
          ..clear()
          ..addAll(chunk.lines);
        _dropped = chunk.dropped;
        _alive = chunk.alive;
      });
      _autoScroll();
      await _lisp.subscribeConsole(true);
    } catch (_) {
      // No link yet — the stream will fill in once one comes up.
    }
    _sub = _lisp.console.listen((l) {
      if (!mounted) return;
      setState(() {
        _lines.add(l);
        if (l.kind == 'print') _alive = true;
        while (_lines.length > 500) {
          _lines.removeAt(0);
          _dropped++;
        }
      });
      _autoScroll();
    });
  }

  /// Follow the tail only while the user hasn't scrolled up to read something.
  void _trackPin() {
    if (!_scroll.hasClients) return;
    _pinned = _scroll.offset >= _scroll.position.maxScrollExtent - 24;
  }

  void _autoScroll() {
    if (!_pinned) return;
    WidgetsBinding.instance.addPostFrameCallback((_) {
      if (_scroll.hasClients) {
        _scroll.jumpTo(_scroll.position.maxScrollExtent);
      }
    });
  }

  @override
  void dispose() {
    _sub?.cancel();
    unawaited(_lisp.subscribeConsole(false).catchError((_) {}));
    if (_raw) unawaited(_lisp.setConsoleDebug(false).catchError((_) => const <int, int>{}));
    _scroll.dispose();
    super.dispose();
  }

  Future<void> _clear() async {
    setState(() {
      _lines.clear();
      _dropped = 0;
    });
    try {
      await _lisp.clearConsole();
    } catch (_) {}
  }

  Future<void> _copy() async {
    await Clipboard.setData(
        ClipboardData(text: [for (final l in _lines) l.text].join('\n')));
    if (!mounted) return;
    ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(content: Text(t(context, 'lisp.console.copied'))));
  }

  Future<void> _toggleRaw(bool on) async {
    setState(() => _raw = on);
    try {
      final hist = await _lisp.setConsoleDebug(on);
      if (!mounted || !on || hist.isEmpty) return;
      // The histogram is the diagnostic: on the head-unit path it is dominated
      // by the P4's own 10 Hz polls, which proves the tap is alive even when
      // the script itself is silent.
      final top = hist.entries.toList()
        ..sort((a, b) => b.value.compareTo(a.value));
      setState(() => _lines.add(LispConsoleLine(
            seq: -1,
            tMs: 0,
            kind: 'marker',
            text: '— unmatched ids: '
                '${top.take(6).map((e) => '${e.key}×${e.value}').join(', ')} —',
          )));
    } catch (_) {}
  }

  @override
  Widget build(BuildContext context) {
    final theme = Theme.of(context);
    return Column(
      crossAxisAlignment: CrossAxisAlignment.stretch,
      children: [
        Row(
          children: [
            const SizedBox(width: 12),
            Expanded(
              child: Text(t(context, 'lisp.console.title'),
                  style: theme.textTheme.titleSmall),
            ),
            TextButton(
                onPressed: _lines.isEmpty ? null : _copy,
                child: Text(t(context, 'lisp.console.copy'))),
            TextButton(
                onPressed: _lines.isEmpty ? null : _clear,
                child: Text(t(context, 'lisp.console.clear'))),
          ],
        ),
        if (_dropped > 0)
          Padding(
            padding: const EdgeInsets.symmetric(horizontal: 12),
            child: Text(
              t(context, 'lisp.console.dropped').replaceAll('{n}', '$_dropped'),
              style: theme.textTheme.bodySmall
                  ?.copyWith(color: theme.colorScheme.outline),
            ),
          ),
        const Divider(height: 1),
        Expanded(
          child: _lines.isEmpty
              ? Center(
                  child: Padding(
                    padding: const EdgeInsets.all(24),
                    child: Text(
                      t(context,
                          _alive ? 'lisp.console.empty' : 'lisp.console.dead'),
                      textAlign: TextAlign.center,
                      style: theme.textTheme.bodySmall
                          ?.copyWith(color: theme.colorScheme.outline),
                    ),
                  ),
                )
              : ListView.builder(
                  controller: _scroll,
                  padding: const EdgeInsets.symmetric(
                      horizontal: 12, vertical: 8),
                  itemCount: _lines.length,
                  itemBuilder: (_, i) {
                    final l = _lines[i];
                    return Text(
                      l.text,
                      style: TextStyle(
                        fontFamily: 'monospace',
                        fontSize: 12,
                        height: 1.35,
                        color: switch (l.kind) {
                          'marker' => theme.colorScheme.outline,
                          'raw' => theme.colorScheme.tertiary,
                          _ => null,
                        },
                      ),
                    );
                  },
                ),
        ),
        SafeArea(
          top: false,
          child: SwitchListTile(
            dense: true,
            title: Text(t(context, 'lisp.console.raw'),
                style: theme.textTheme.bodySmall),
            value: _raw,
            onChanged: _toggleRaw,
          ),
        ),
      ],
    );
  }
}
