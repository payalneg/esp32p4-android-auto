/// The tools the model may call, and their JSON schemas.
///
/// Two rules run through all of it:
///
///  * **A tool never throws at the model.** Every failure becomes
///    `{"ok": false, "error": …}` with enough detail to act on. That return
///    value IS the self-correction loop — an exception would just end the turn.
///  * **What isn't here can't happen.** No raw VESC commands, no `conf-set`,
///    no current/duty/RPM, no REPL, no phone filesystem, no network. The agent
///    can only reach the motor controller through a script the user approved
///    on screen.
library;

import 'dart:async';
import 'dart:convert';

import 'package:crypto/crypto.dart' show sha256;

import 'agent_budget.dart';
import 'cancel_token.dart';
import 'lisp_device.dart';
import 'lisp_lint.dart';
import 'lisp_patch.dart';
import 'verify.dart';

/// Everything a tool is allowed to reach.
class ToolCtx {
  ToolCtx({
    required this.dev,
    required this.counters,
    required this.budget,
    required this.cancel,
    required this.confirm,
    required this.onProgress,
    required this.getWorking,
    required this.setWorking,
  });

  final LispDevice dev;
  final AgentUsageCounters counters;
  final AgentBudget budget;
  final CancelToken cancel;

  /// Ask the user to approve a device-mutating call. Resolves false on
  /// decline or timeout.
  final Future<bool> Function(ConfirmRequest) confirm;
  final void Function(double) onProgress;

  /// The working copy IS the editor buffer — one source of truth, so the user
  /// watches edits land and can hand-edit between steps.
  final String Function() getWorking;
  final void Function(String) setWorking;

  /// The script as read from the device at session start; the rollback target.
  String? pristine;

  /// Last text actually written to the VESC, so a no-op reflash is refused.
  String? lastFlashed;

  /// Set once the console has proven it carries output on this link.
  bool consoleEverAlive = false;
}

class ConfirmRequest {
  const ConfirmRequest({
    required this.callId,
    required this.titleKey,
    required this.rationale,
    required this.diffSummary,
    required this.warnings,
    required this.run,
  });
  final String callId;
  final String titleKey;
  final String rationale;
  final String diffSummary;
  final List<String> warnings;
  final bool run;
}

class ToolSpec {
  const ToolSpec({
    required this.name,
    required this.description,
    required this.parameters,
    required this.run,
    this.touchesDevice = false,
    this.needsConfirmation = false,
  });

  final String name;
  final String description;
  final Map<String, dynamic> parameters;
  final bool touchesDevice;
  final bool needsConfirmation;
  final Future<Map<String, dynamic>> Function(
      ToolCtx ctx, Map<String, dynamic> args, String callId) run;

  Map<String, dynamic> toWire({bool strict = false}) => {
        'type': 'function',
        'function': {
          'name': name,
          'description': description,
          'parameters': parameters,
          if (strict) 'strict': true,
        },
      };
}

Map<String, dynamic> _obj(Map<String, dynamic> props, List<String> required) =>
    {
      'type': 'object',
      'properties': props,
      'required': required,
      'additionalProperties': false,
    };

Map<String, dynamic> _str(String desc) =>
    {'type': 'string', 'description': desc};
Map<String, dynamic> _int(String desc) =>
    {'type': 'integer', 'description': desc};
Map<String, dynamic> _bool(String desc) =>
    {'type': 'boolean', 'description': desc};
Map<String, dynamic> _strArray(String desc) =>
    {'type': 'array', 'items': {'type': 'string'}, 'description': desc};

Map<String, dynamic> _err(String code, String detail, {String? hint}) =>
    {'ok': false, 'error': code, 'detail': detail, if (hint != null) 'hint': hint};

/// Gutter-formatted excerpt. The contract says SEARCH must not include the
/// gutter, and that only works if the gutter is unmistakable.
String _withLineNumbers(List<String> lines, int startLine) {
  final b = StringBuffer();
  for (var i = 0; i < lines.length; i++) {
    b.writeln('${(startLine + i).toString().padLeft(4)}| ${lines[i]}');
  }
  return b.toString();
}

class ToolRegistry {
  ToolRegistry({required this.writeEnabled});

  /// Phase-4 master switch: with it off the agent can read, analyse and edit
  /// the working copy, but the user does every flash by hand.
  final bool writeEnabled;

  late final Map<String, ToolSpec> _byName = {
    for (final t in _all) t.name: t,
  };

  ToolSpec? operator [](String name) => _byName[name];

  /// Schemas to send. [consoleAvailable] drops `read_console` entirely when
  /// the channel is known-dead — an always-empty tool poisons the loop.
  List<Map<String, dynamic>> schemas(
      {required bool consoleAvailable, bool strict = false}) {
    return [
      for (final t in _all)
        if (consoleAvailable || t.name != 'read_console')
          t.toWire(strict: strict),
    ];
  }

  late final List<ToolSpec> _all = [
    _readScript,
    _readLines,
    _grepScript,
    _lintScript,
    _getStats,
    _readConsole,
    _getLinkInfo,
    _applyPatch,
    _writeScript,
    _revertWorkingCopy,
    if (writeEnabled) ...[
      _flashScript,
      _setRunning,
      _revertToFlashed,
    ],
    _finish,
  ];

  // ---- read-only ---------------------------------------------------------

  final _readScript = ToolSpec(
    name: 'read_script',
    description:
        'Read the LISP script currently stored on the VESC into the working '
        'copy. Returns metadata only — use read_lines to see the text, so a '
        'full re-read does not burn context.',
    touchesDevice: true,
    parameters: _obj({
      'force': _bool('Re-read from the device even if a working copy exists.'),
    }, []),
    run: (ctx, args, _) async {
      if (!ctx.dev.connected) {
        return _err('no_link', 'No connection to the VESC.');
      }
      final force = args['force'] as bool? ?? false;
      if (!force && ctx.getWorking().trim().isNotEmpty) {
        final text = ctx.getWorking();
        return {
          'ok': true,
          'loaded_from': 'working_copy',
          'bytes': utf8.encode(text).length,
          'lines': text.split('\n').length,
          'sha256': sha256.convert(utf8.encode(text)).toString().substring(0, 12),
        };
      }
      try {
        final code = await ctx.dev.readCode(onProgress: ctx.onProgress);
        ctx.setWorking(code);
        ctx.pristine ??= code;
        return {
          'ok': true,
          'loaded_from': 'device',
          'bytes': utf8.encode(code).length,
          'lines': code.split('\n').length,
          'sha256':
              sha256.convert(utf8.encode(code)).toString().substring(0, 12),
        };
      } catch (e) {
        return _err('read_failed', '$e');
      }
    },
  );

  final _readLines = ToolSpec(
    name: 'read_lines',
    description:
        'Show a range of the working copy with a "NNNN| " line-number gutter. '
        'The gutter is display only — never include it in a SEARCH block.',
    parameters: _obj({
      'start': _int('First line, 1-based.'),
      'count': _int('How many lines (1-400).'),
    }, ['start']),
    run: (ctx, args, _) async {
      final lines = ctx.getWorking().split('\n');
      final start = ((args['start'] as num?)?.toInt() ?? 1).clamp(1, lines.length);
      final count = ((args['count'] as num?)?.toInt() ?? 120).clamp(1, 400);
      final end = (start - 1 + count).clamp(0, lines.length);
      return {
        'ok': true,
        'start': start,
        'end': end,
        'total_lines': lines.length,
        'text': _withLineNumbers(lines.sublist(start - 1, end), start),
      };
    },
  );

  final _grepScript = ToolSpec(
    name: 'grep_script',
    description: 'Find lines in the working copy matching a pattern.',
    parameters: _obj({
      'pattern': _str('Text or regular expression to look for.'),
      'is_regex': _bool('Treat the pattern as a regular expression.'),
      'context': _int('Lines of context around each hit (0-5).'),
    }, ['pattern']),
    run: (ctx, args, _) async {
      final pattern = args['pattern'] as String? ?? '';
      if (pattern.isEmpty) return _err('bad_pattern', 'Empty pattern.');
      final ctxLines = ((args['context'] as num?)?.toInt() ?? 0).clamp(0, 5);
      final lines = ctx.getWorking().split('\n');
      bool Function(String) test;
      if (args['is_regex'] == true) {
        try {
          final re = RegExp(pattern);
          test = re.hasMatch;
        } catch (e) {
          return _err('bad_regex', '$e');
        }
      } else {
        test = (l) => l.contains(pattern);
      }
      final hits = <Map<String, dynamic>>[];
      for (var i = 0; i < lines.length && hits.length < 60; i++) {
        if (!test(lines[i])) continue;
        final from = (i - ctxLines).clamp(0, lines.length - 1);
        final to = (i + ctxLines + 1).clamp(0, lines.length);
        hits.add({
          'line': i + 1,
          'text': ctxLines == 0
              ? lines[i]
              : _withLineNumbers(lines.sublist(from, to), from + 1),
        });
      }
      return {'ok': true, 'matches': hits, 'truncated': hits.length >= 60};
    },
  );

  final _lintScript = ToolSpec(
    name: 'lint_script',
    description:
        'Check the working copy against the VESC LispBM rules that fail '
        'silently on hardware (@const placement, mutable defs, use before '
        'bind, size, quick-action panel frames). flash_script refuses to run '
        'while this reports errors.',
    parameters: _obj({}, []),
    run: (ctx, args, _) async => lintLisp(ctx.getWorking()).toJson(),
  );

  final _getStats = ToolSpec(
    name: 'get_stats',
    description:
        'Sample the LISP runtime: cpu/heap/mem/stack, the done-context string '
        '(where evaluation errors surface) and global variable values. The '
        'firmware returns AT MOST 18 bindings, so pick debug globals with '
        'that in mind.',
    touchesDevice: true,
    parameters: _obj({
      'samples': _int('How many snapshots to take (1-5).'),
      'interval_ms': _int('Delay between snapshots (200-2000 ms).'),
    }, []),
    run: (ctx, args, _) async {
      if (!ctx.dev.connected) return _err('no_link', 'No connection.');
      final n = ((args['samples'] as num?)?.toInt() ?? 1).clamp(1, 5);
      final gap =
          ((args['interval_ms'] as num?)?.toInt() ?? 500).clamp(200, 2000);
      final out = <Map<String, dynamic>>[];
      var truncated = false;
      for (var i = 0; i < n; i++) {
        ctx.cancel.throwIfCancelled();
        if (i > 0) {
          await Future<void>.delayed(Duration(milliseconds: gap));
        }
        try {
          final s = await ctx.dev.statsOnce();
          truncated = truncated || s.bindings.length >= 18;
          out.add({
            'cpu': s.cpu,
            'heap': s.heap,
            'mem': s.mem,
            'stack': s.stack,
            'done_ctx': s.doneCtx,
            'bindings': {for (final b in s.bindings) b.name: b.value},
          });
        } catch (e) {
          return out.isEmpty
              ? _err('timeout', '$e')
              : {'ok': true, 'samples': out, 'partial': true};
        }
      }
      return {
        'ok': true,
        'samples': out,
        'truncated_at_18': truncated,
      };
    },
  );

  final _readConsole = ToolSpec(
    name: 'read_console',
    description:
        'Read asynchronous (print ...) output from the running script. '
        'channel_alive is false when this link has never carried any output — '
        'that means "unavailable", not "the script is quiet".',
    touchesDevice: true,
    parameters: _obj({
      'since_seq': _int('Resume from this sequence number (0 = from the start).'),
      'max_lines': _int('At most this many lines (1-200).'),
      'wait_ms': _int('Wait up to this long for new output (0-5000 ms).'),
    }, []),
    run: (ctx, args, _) async {
      final since = (args['since_seq'] as num?)?.toInt() ?? 0;
      final maxLines =
          ((args['max_lines'] as num?)?.toInt() ?? 200).clamp(1, 200);
      final wait = ((args['wait_ms'] as num?)?.toInt() ?? 0).clamp(0, 5000);
      try {
        var chunk = await ctx.dev.readConsole(sinceSeq: since, maxLines: maxLines);
        if (chunk.lines.isEmpty && wait > 0) {
          await Future<void>.delayed(Duration(milliseconds: wait));
          ctx.cancel.throwIfCancelled();
          chunk = await ctx.dev.readConsole(sinceSeq: since, maxLines: maxLines);
        }
        ctx.consoleEverAlive = ctx.consoleEverAlive || chunk.alive;
        return {
          'ok': true,
          'lines': [
            for (final l in chunk.lines) {'seq': l.seq, 'text': l.text}
          ],
          'next_seq': chunk.nextSeq,
          'dropped': chunk.dropped,
          'channel_alive': chunk.alive,
        };
      } catch (e) {
        return _err('console_failed', '$e');
      }
    },
  );

  final _getLinkInfo = ToolSpec(
    name: 'get_link_info',
    description:
        'How the phone is talking to the VESC, and what budget is left.',
    parameters: _obj({}, []),
    run: (ctx, args, _) async => {
          'ok': true,
          'link': ctx.dev.linkLabel,
          'connected': ctx.dev.connected,
          'mtu': ctx.dev.mtu,
          'lisp_max_bytes': kLispMaxBytes,
          'console_alive': ctx.consoleEverAlive,
          'flashes_used': ctx.counters.flashes,
          'flashes_remaining':
              (ctx.budget.maxFlashes - ctx.counters.flashes).clamp(0, 999),
          'has_pristine_backup': ctx.pristine != null,
        },
  );

  // ---- working-copy edits (never touch hardware) -------------------------

  final _applyPatch = ToolSpec(
    name: 'apply_patch',
    description:
        'Edit the working copy with SEARCH/REPLACE blocks:\n'
        '<<<<<<< SEARCH\n(exact existing text)\n=======\n(replacement)\n'
        '>>>>>>> REPLACE\n'
        'SEARCH must match byte-exactly and exactly once — include '
        'surrounding lines until it is unique, and never include the '
        '"NNNN| " gutter from read_lines. An empty SEARCH is rejected. All '
        'blocks apply together or not at all.',
    parameters: _obj({
      'patch': _str('One or more SEARCH/REPLACE blocks.'),
      'intent': _str('One sentence describing the change.'),
    }, ['patch', 'intent']),
    run: (ctx, args, _) async {
      final res = applyPatch(ctx.getWorking(), args['patch'] as String? ?? '');
      if (!res.ok) return res.toJson();
      ctx.setWorking(res.text!);
      final lint = lintLisp(res.text!);
      return {
        ...res.toJson(),
        'new_lines': res.text!.split('\n').length,
        'new_bytes': utf8.encode(res.text!).length,
        'lint': {
          'errors': [for (final i in lint.errors) i.toJson()],
          'warnings': [for (final i in lint.warnings) i.toJson()],
        },
      };
    },
  );

  final _writeScript = ToolSpec(
    name: 'write_script',
    description:
        'Replace the entire working copy. Only for a new script or a genuine '
        'rewrite — for edits use apply_patch, which is cheaper and safer.',
    parameters: _obj({
      'content': _str('The complete new script.'),
      'reason': _str('Why a full rewrite rather than a patch.'),
    }, ['content', 'reason']),
    run: (ctx, args, _) async {
      final content = args['content'] as String? ?? '';
      final reason = (args['reason'] as String? ?? '').trim();
      if (content.trim().isEmpty) {
        return _err('empty', 'Refusing to replace the script with nothing.');
      }
      if (ctx.getWorking().trim().isNotEmpty && reason.isEmpty) {
        return _err('needs_reason',
            'A full rewrite of a non-empty script needs a reason.',
            hint: 'Prefer apply_patch for targeted edits.');
      }
      final before = ctx.getWorking();
      ctx.setWorking(content);
      final lint = lintLisp(content);
      return {
        'ok': true,
        'diff': unifiedDiff(before, content),
        'new_lines': content.split('\n').length,
        'new_bytes': utf8.encode(content).length,
        'lint': {
          'errors': [for (final i in lint.errors) i.toJson()],
          'warnings': [for (final i in lint.warnings) i.toJson()],
        },
      };
    },
  );

  final _revertWorkingCopy = ToolSpec(
    name: 'revert_working_copy',
    description:
        'Throw away edits and restore the script as it was read from the '
        'device. Does not touch the device.',
    parameters: _obj({}, []),
    run: (ctx, args, _) async {
      if (ctx.pristine == null) {
        return _err('no_backup', 'Nothing has been read from the device yet.');
      }
      ctx.setWorking(ctx.pristine!);
      return {'ok': true, 'lines': ctx.pristine!.split('\n').length};
    },
  );

  // ---- device mutation (always confirmed) --------------------------------

  final _flashScript = ToolSpec(
    name: 'flash_script',
    description:
        'Write the working copy to the VESC and verify it. Requires the user '
        'to tap Confirm. Refuses while lint_script reports errors. After '
        'flashing it automatically samples the runtime and reports whether '
        'the script actually runs — read that report, do not assume success.',
    touchesDevice: true,
    needsConfirmation: true,
    parameters: _obj({
      'run': _bool('Start the script after writing.'),
      'rationale': _str('What this change is meant to achieve.'),
      'expect_prints': _strArray(
          'Substrings expected in console output within a few seconds.'),
      'moving_globals': _strArray(
          'Globals that MUST change between samples if the script is alive.'),
    }, ['run', 'rationale']),
    run: (ctx, args, callId) async {
      if (!ctx.dev.connected) return _err('no_link', 'No connection.');

      final code = ctx.getWorking();
      if (code.trim().isEmpty) {
        return _err('empty', 'The working copy is empty.');
      }

      // Hard gate: the lint rules exist because these failures ack as success
      // and leave a half-dead script in charge of a motor.
      final lint = lintLisp(code);
      if (lint.hasErrors) {
        return {
          'ok': false,
          'error': 'lint_blocked',
          'issues': [for (final i in lint.errors) i.toJson()],
          'hint': 'Fix these before flashing — they fail silently on hardware.',
        };
      }
      if (lint.packedBytes + 100 > kLispMaxBytes) {
        return _err('too_big', '${lint.packedBytes} B packed.');
      }
      if (!ctx.budget.canFlash(ctx.counters)) {
        return _err('flash_budget',
            'Flash budget for this session is used up '
            '(${ctx.counters.flashes}/${ctx.budget.maxFlashes}).');
      }
      if (ctx.lastFlashed == code) {
        return _err('no_change',
            'This exact script is already on the device.');
      }

      final run = args['run'] as bool? ?? false;
      final base = ctx.lastFlashed ?? ctx.pristine ?? '';
      final diff = unifiedDiff(base, code, maxLines: 200);
      final approved = await ctx.confirm(ConfirmRequest(
        callId: callId,
        titleKey: run ? 'agent.confirm.flashRun' : 'agent.confirm.flash',
        rationale: args['rationale'] as String? ?? '',
        diffSummary: _diffSummary(diff, code),
        warnings: [for (final w in lint.warnings) w.message],
        run: run,
      ));
      if (!approved) {
        return _err('user_declined', 'The user did not approve the flash.');
      }

      ctx.counters.flashes++;
      try {
        final report = await flashAndVerify(
          ctx.dev,
          code,
          run: run,
          expectPrints: [
            for (final e in (args['expect_prints'] as List?) ?? const [])
              '$e'
          ],
          movingGlobals: [
            for (final e in (args['moving_globals'] as List?) ?? const [])
              '$e'
          ],
          cancel: ctx.cancel,
          onProgress: ctx.onProgress,
        );
        ctx.lastFlashed = code;
        ctx.consoleEverAlive = ctx.consoleEverAlive || report.consoleAlive;
        return {'ok': true, ...report.toJson()};
      } catch (e) {
        return _err('flash_failed', '$e');
      }
    },
  );

  final _setRunning = ToolSpec(
    name: 'set_running',
    description:
        'Start or stop the script stored on the VESC. Stopping is always '
        'allowed; starting requires the user to tap Confirm.',
    touchesDevice: true,
    needsConfirmation: true,
    parameters: _obj({
      'run': _bool('true starts the script, false stops it.'),
      'reason': _str('Why.'),
    }, ['run', 'reason']),
    run: (ctx, args, callId) async {
      if (!ctx.dev.connected) return _err('no_link', 'No connection.');
      final run = args['run'] as bool? ?? false;
      if (run) {
        final approved = await ctx.confirm(ConfirmRequest(
          callId: callId,
          titleKey: 'agent.confirm.start',
          rationale: args['reason'] as String? ?? '',
          diffSummary: '',
          warnings: const [],
          run: true,
        ));
        if (!approved) {
          return _err('user_declined', 'The user did not approve starting.');
        }
      }
      try {
        await ctx.dev.setRunning(run);
        await Future<void>.delayed(const Duration(seconds: 1));
        final s = await ctx.dev.statsOnce();
        return {
          'ok': true,
          'running': run,
          'stats_after': {
            'cpu': s.cpu,
            'heap': s.heap,
            'done_ctx': s.doneCtx,
            'binding_count': s.bindings.length,
          },
        };
      } catch (e) {
        return _err('set_running_failed', '$e');
      }
    },
  );

  final _revertToFlashed = ToolSpec(
    name: 'revert_to_flashed_backup',
    description:
        'Put the script that was on the device at the start of this session '
        'back on it. The safe direction — use it if a change made things '
        'worse.',
    touchesDevice: true,
    needsConfirmation: true,
    parameters: _obj({
      'run': _bool('Start the restored script.'),
    }, ['run']),
    run: (ctx, args, callId) async {
      if (ctx.pristine == null) {
        return _err('no_backup', 'No original script was captured.');
      }
      if (!ctx.dev.connected) return _err('no_link', 'No connection.');
      if (!ctx.budget.canFlash(ctx.counters)) {
        return _err('flash_budget', 'Flash budget used up.');
      }
      final run = args['run'] as bool? ?? true;
      final approved = await ctx.confirm(ConfirmRequest(
        callId: callId,
        titleKey: 'agent.confirm.revert',
        rationale: 'Restore the script captured at the start of the session.',
        diffSummary: '',
        warnings: const [],
        run: run,
      ));
      if (!approved) return _err('user_declined', 'Not approved.');

      ctx.counters.flashes++;
      try {
        final report = await flashAndVerify(ctx.dev, ctx.pristine!,
            run: run, cancel: ctx.cancel, onProgress: ctx.onProgress);
        ctx.setWorking(ctx.pristine!);
        ctx.lastFlashed = ctx.pristine;
        return {'ok': true, 'restored': true, ...report.toJson()};
      } catch (e) {
        return _err('flash_failed', '$e');
      }
    },
  );

  final _finish = ToolSpec(
    name: 'finish',
    description:
        'End the task and summarise. Call this when the work is done, when '
        'you are blocked, or when you need something only the user can do.',
    parameters: _obj({
      'status': {
        'type': 'string',
        'enum': ['solved', 'blocked', 'needs_user', 'gave_up'],
        'description': 'How the task ended.',
      },
      'summary': _str('What was done and what the hardware reported.'),
      'next_steps': _strArray('Anything left for the user to do.'),
    }, ['status', 'summary']),
    run: (ctx, args, _) async => {'ok': true, 'acknowledged': true},
  );
}

String _diffSummary(String diff, String code) {
  var add = 0, del = 0;
  for (final l in diff.split('\n')) {
    if (l.startsWith('+')) add++;
    if (l.startsWith('-')) del++;
  }
  final kb = (utf8.encode(code).length / 1024).toStringAsFixed(1);
  return '+$add −$del · $kb KB';
}
