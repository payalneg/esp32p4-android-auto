/// Post-flash verification — the part that makes this an agent rather than a
/// code generator with a serial port.
///
/// It runs INSIDE `flash_script`, so the model cannot skip it and cannot claim
/// success it didn't observe. The protocol acks prove only that bytes reached
/// flash; every failure mode this project has actually hit (defuns outside
/// `@const` OOMing the heap, a handler thread dying on an unbound symbol) acks
/// perfectly and then does nothing.
///
/// What the hardware will actually tell us, best first:
///   1. `doneCtx` from GET_STATS — where LispBM reports eval errors.
///   2. Globals moving between samples — proof the script is still executing.
///   3. Console output, when the link carries it at all.
///   4. Heap climbing — the signature of the cons-heap OOM.
///
/// Three samples, not one: with the head unit polling the VESC at 10 Hz our
/// stats request can be answered by someone else's reply, and a single
/// snapshot can look healthy by luck.
library;

import 'dart:async';

import '../ble/lisp_models.dart';
import 'cancel_token.dart';
import 'lisp_device.dart';

enum Verdict {
  ok,
  flashedNotRun,
  evalError,
  notRunning,
  oomSuspect,
  cpuSaturated,
  expectMissed,
  inconclusive,
  linkLost,
}

/// `doneCtx` values that are not failures. LispBM reports the context that
/// finished last; a plain "done" is normal.
const _benignDoneCtx = {'', 'done', 'ok', 'nil'};

class VerifyReport {
  const VerifyReport({
    required this.verdict,
    required this.pass,
    required this.reasons,
    required this.samples,
    this.before,
    this.console = const [],
    this.consoleAlive = false,
    this.consoleDropped = 0,
    this.movingGlobals = const {},
    this.stoppedAfterFailure = false,
    this.hint,
  });

  final Verdict verdict;
  final bool pass;
  final List<String> reasons;
  final List<LispStats> samples;
  final LispStats? before;
  final List<String> console;
  final bool consoleAlive;
  final int consoleDropped;

  /// Global name -> its value in each sample, so "did it move?" is visible
  /// rather than asserted.
  final Map<String, List<double>> movingGlobals;
  final bool stoppedAfterFailure;

  /// Project knowledge attached at the moment it is relevant.
  final String? hint;

  Map<String, dynamic> toJson() => {
        'verdict': verdict.name,
        'pass': pass,
        'reasons': reasons,
        if (samples.isNotEmpty) 'done_ctx': samples.last.doneCtx,
        'samples': [
          for (final s in samples)
            {
              'cpu': _r(s.cpu),
              'heap': _r(s.heap),
              'mem': _r(s.mem),
              'stack': _r(s.stack),
              'done_ctx': s.doneCtx,
              'binding_count': s.bindings.length,
            }
        ],
        if (movingGlobals.isNotEmpty)
          'moving_globals': {
            for (final e in movingGlobals.entries)
              e.key: [for (final v in e.value) _r(v)]
          },
        'console': console,
        'console_alive': consoleAlive,
        if (consoleDropped > 0) 'console_dropped': consoleDropped,
        if (stoppedAfterFailure) 'stopped_after_failure': true,
        if (hint != null) 'hint': hint,
      };

  static double _r(double v) => double.parse(v.toStringAsFixed(2));
}

/// Flash [code] and decide whether it actually runs.
///
/// Sequence: snapshot → stop → erase+write → start → settle → 3 samples →
/// verdict → stop again if it failed.
Future<VerifyReport> flashAndVerify(
  LispDevice dev,
  String code, {
  required bool run,
  List<String> expectPrints = const [],
  List<String> movingGlobals = const [],
  bool autoStopOnFail = true,
  CancelToken? cancel,
  void Function(double)? onProgress,
  Duration settle = const Duration(milliseconds: 800),
  List<Duration> sampleAt = const [
    Duration(milliseconds: 1000),
    Duration(milliseconds: 2000),
    Duration(milliseconds: 3500),
  ],
}) async {
  LispStats? before;
  try {
    before = await dev.statsOnce();
  } catch (_) {
    // Not fatal: we may be flashing over a script that already died.
  }
  try {
    await dev.clearConsole();
  } catch (_) {}

  cancel?.throwIfCancelled();
  await dev.upload(code, run: run, stopFirst: true, onProgress: onProgress);

  if (!run) {
    return VerifyReport(
      verdict: Verdict.flashedNotRun,
      pass: true,
      reasons: const ['written to flash but not started'],
      samples: const [],
      before: before,
    );
  }

  final samples = <LispStats>[];
  final console = <String>[];
  var consoleAlive = false;
  var consoleDropped = 0;
  var consoleSeq = 0;

  final clock = Stopwatch()..start();
  await _wait(settle, cancel);

  for (final at in sampleAt) {
    final remaining = at - clock.elapsed;
    if (remaining > Duration.zero) await _wait(remaining, cancel);
    try {
      samples.add(await dev.statsOnce());
    } catch (_) {
      // A dropped sample is data too — it usually means the link went away.
    }
    try {
      final chunk = await dev.readConsole(sinceSeq: consoleSeq);
      consoleSeq = chunk.nextSeq;
      consoleAlive = consoleAlive || chunk.alive;
      consoleDropped = chunk.dropped;
      for (final l in chunk.lines) {
        if (l.kind == 'print') console.add(l.text);
      }
    } catch (_) {}
  }

  final report = _decide(
    before: before,
    samples: samples,
    console: console,
    consoleAlive: consoleAlive,
    consoleDropped: consoleDropped,
    expectPrints: expectPrints,
    watchGlobals: movingGlobals,
  );

  if (!report.pass && autoStopOnFail) {
    // A script that failed verification does not get to keep running on a
    // motor controller.
    try {
      await dev.setRunning(false);
      return report._withStopped();
    } catch (_) {}
  }
  return report;
}

Future<void> _wait(Duration d, CancelToken? cancel) async {
  if (cancel == null) {
    await Future<void>.delayed(d);
    return;
  }
  await Future.any([Future<void>.delayed(d), cancel.whenCancelled]);
  cancel.throwIfCancelled();
}

VerifyReport _decide({
  required LispStats? before,
  required List<LispStats> samples,
  required List<String> console,
  required bool consoleAlive,
  required int consoleDropped,
  required List<String> expectPrints,
  required List<String> watchGlobals,
}) {
  final tracked = <String, List<double>>{};
  for (final name in watchGlobals) {
    tracked[name] = [
      for (final s in samples)
        s.bindings
            .firstWhere((b) => b.name == name,
                orElse: () => const LispBinding('', double.nan))
            .value
    ];
  }

  VerifyReport build(Verdict v, bool pass, List<String> reasons,
          {String? hint}) =>
      VerifyReport(
        verdict: v,
        pass: pass,
        reasons: reasons,
        samples: samples,
        before: before,
        console: console,
        consoleAlive: consoleAlive,
        consoleDropped: consoleDropped,
        movingGlobals: tracked,
        hint: hint,
      );

  if (samples.isEmpty) {
    return build(Verdict.linkLost, false,
        ['no stats came back after the flash — the link may have dropped']);
  }

  final last = samples.last;

  // 1. An eval error is the highest-signal thing the hardware produces. Hand
  //    doneCtx back verbatim; it names the failure.
  final ctx = last.doneCtx.trim();
  if (ctx.isNotEmpty && !_benignDoneCtx.contains(ctx.toLowerCase())) {
    return build(Verdict.evalError, false,
        ['the script reported an evaluation error: "$ctx"'],
        hint: _hintFor(ctx, last));
  }

  // 2. Nothing bound at all: the script isn't loaded.
  if (last.bindings.isEmpty) {
    return build(Verdict.notRunning, false, [
      'GET_STATS returned no bindings, so no script appears to be loaded'
    ], hint: 'Check that the upload succeeded and the script was started.');
  }

  // 3. A watched global that never moves means execution stopped.
  for (final e in tracked.entries) {
    final vals = e.value.where((v) => !v.isNaN).toList();
    if (vals.length < 2) continue;
    if (vals.every((v) => v == vals.first)) {
      return build(Verdict.notRunning, false, [
        "'${e.key}' did not change across ${vals.length} samples "
            '(${vals.first}) — the loop that updates it is not running'
      ],
          hint: 'A thread that references a symbol bound later in the file '
              'dies silently at load. Check the order of spawns.');
    }
  }

  // 4. Heap climbing monotonically is the cons-heap OOM signature.
  if (samples.length >= 3) {
    final h = [for (final s in samples) s.heap];
    final rising = h.first < h[1] && h[1] < h.last;
    if (rising && (h.last - h.first) > 3.0) {
      return build(Verdict.oomSuspect, false, [
        'heap climbed from ${h.first.toStringAsFixed(1)}% to '
            '${h.last.toStringAsFixed(1)}% over ${samples.length} samples'
      ],
          hint: 'Rising heap usually means definitions are living in the cons '
              'heap — check that every defun is inside the @const block.');
    }
  }

  // 5. Expected prints. Never punish the model for a channel that isn't there.
  final missing = [
    for (final want in expectPrints)
      if (!console.any((l) => l.contains(want))) want
  ];
  if (missing.isNotEmpty) {
    if (!consoleAlive) {
      return build(Verdict.inconclusive, true, [
        'expected output ${missing.join(', ')} was not seen, but this link '
            'has never carried print output — treat as unverified, not failed'
      ],
          hint: 'Use a debug global and moving_globals instead of prints on '
              'this link.');
    }
    return build(Verdict.expectMissed, false,
        ['expected console output not seen: ${missing.join(', ')}']);
  }

  // 6. Saturated CPU is worth flagging but is not a failure.
  if (samples.every((s) => s.cpu > 95)) {
    return build(Verdict.cpuSaturated, true,
        ['the script is running but CPU is pinned above 95%']);
  }

  return build(Verdict.ok, true, ['script is running and reports no errors']);
}

/// Turn the hardware's own words into the project's accumulated knowledge, at
/// the moment it is actionable.
String? _hintFor(String doneCtx, LispStats s) {
  final c = doneCtx.toLowerCase();
  if (c.contains('out_of_memory') || c.contains('out of memory')) {
    return 'heap is at ${s.heap.toStringAsFixed(1)}%. This is almost always '
        'defuns outside the @const block: wrap ALL of them in one '
        '@const-start/@const-end, keep setq\'d defs and bufcreate buffers '
        'above it, and watch out for large quoted literals which cost heap '
        'at parse time.';
  }
  if (c.contains('variable_not_bound') || c.contains('not_bound')) {
    return 'A symbol was used before it was defined. Top-level statements run '
        'as the file loads — move spawns and event-enable below every '
        'definition they reference.';
  }
  if (c.contains('type_error')) {
    return 'Check the argument types at the reported context — a flashed '
        '(@const) buffer is read-only, so bufset on it fails this way.';
  }
  if (c.contains('eval_error')) {
    return 'A form failed to evaluate. Read back the region you changed and '
        'check the parentheses and argument counts.';
  }
  return null;
}

extension on VerifyReport {
  VerifyReport _withStopped() => VerifyReport(
        verdict: verdict,
        pass: pass,
        reasons: [...reasons, 'the script was stopped automatically'],
        samples: samples,
        before: before,
        console: console,
        consoleAlive: consoleAlive,
        consoleDropped: consoleDropped,
        movingGlobals: movingGlobals,
        stoppedAfterFailure: true,
        hint: hint,
      );
}
