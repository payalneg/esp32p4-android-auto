/// A VESC that lives in memory.
///
/// The real path costs ~8 s per read and ~15 s per flash, so without this the
/// agent loop and every verification verdict would be untestable. It models
/// the behaviours that matter: acks that succeed while the script is dead,
/// `doneCtx` carrying an eval error, globals that do or don't move, a heap
/// that climbs, and a console that may not exist at all on this link.
library;

import 'package:aa_bridge/agent/lisp_device.dart';
import 'package:aa_bridge/ble/lisp_models.dart';

class FakeLispDevice implements LispDevice {
  FakeLispDevice({
    String? initialCode,
    this.consoleAlive = true,
    this.linkLabel = 'head-unit',
  }) : stored = initialCode ?? '(def x 0)\n';

  @override
  bool connected = true;

  @override
  String linkLabel;

  @override
  int mtu = 247;

  /// What is "on the VESC".
  String stored;
  bool running = false;

  /// Scripted runtime behaviour.
  String doneCtx = '';
  double cpu = 5;
  double heap = 40;

  /// Set to make heap climb between samples (the OOM signature).
  double heapClimbPerSample = 0;

  /// Globals reported by GET_STATS. Values in [movingGlobals] advance on each
  /// sample while the script is running.
  final Map<String, double> globals = {'x': 0};
  final Set<String> movingGlobals = {};

  /// When false, `alive` stays false however much is printed — models a link
  /// that doesn't carry the console at all.
  bool consoleAlive;

  /// Lines the "script" emits on each stats sample while running.
  final List<String> printsPerSample = [];

  final List<LispConsoleLine> _console = [];
  int _seq = 0;

  // Call log, so tests can assert on the sequence (e.g. stop-before-erase).
  final calls = <String>[];
  int statsCalls = 0;

  Object? failNextUpload;
  Object? failStats;

  @override
  Future<String> readCode({void Function(double)? onProgress}) async {
    calls.add('read');
    onProgress?.call(1);
    return stored;
  }

  @override
  Future<void> upload(String code,
      {bool run = false,
      bool stopFirst = true,
      void Function(double)? onProgress}) async {
    calls.add('upload(run=$run,stopFirst=$stopFirst)');
    if (failNextUpload != null) {
      final e = failNextUpload!;
      failNextUpload = null;
      throw e;
    }
    if (stopFirst) running = false;
    stored = code;
    onProgress?.call(1);
    running = run;
  }

  @override
  Future<void> setRunning(bool run) async {
    calls.add('setRunning($run)');
    running = run;
  }

  @override
  Future<LispStats> statsOnce() async {
    statsCalls++;
    if (failStats != null) throw failStats!;
    if (running) {
      for (final name in movingGlobals) {
        globals[name] = (globals[name] ?? 0) + 1;
      }
      heap += heapClimbPerSample;
      for (final line in printsPerSample) {
        _console.add(LispConsoleLine(
            seq: _seq++, tMs: statsCalls * 100, text: line, kind: 'print'));
      }
    }
    return LispStats(
      cpu: cpu,
      heap: heap,
      mem: 30,
      stack: 12,
      doneCtx: doneCtx,
      bindings: [
        // A dead script reports nothing.
        if (running)
          for (final e in globals.entries) LispBinding(e.key, e.value),
      ],
    );
  }

  @override
  Future<LispConsoleChunk> readConsole(
      {int sinceSeq = 0, int maxLines = 200}) async {
    final lines = [
      for (final l in _console)
        if (l.seq >= sinceSeq) l
    ].take(maxLines).toList();
    return LispConsoleChunk(
      lines: lines,
      nextSeq: lines.isEmpty ? sinceSeq : lines.last.seq + 1,
      dropped: 0,
      alive: consoleAlive && _console.isNotEmpty,
    );
  }

  @override
  Future<void> clearConsole() async {
    calls.add('clearConsole');
    _console.clear();
  }
}
