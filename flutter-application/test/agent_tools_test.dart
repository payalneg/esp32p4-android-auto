/// Tool-layer tests — this is the safety boundary, so most of these are about
/// what the agent CANNOT do: flash past the linter, flash without a tap,
/// reflash the same bytes, or exceed its flash budget.
library;

import 'package:aa_bridge/agent/agent_budget.dart';
import 'package:aa_bridge/agent/agent_tools.dart';
import 'package:aa_bridge/agent/cancel_token.dart';
import 'package:flutter_test/flutter_test.dart';

import 'fake_lisp_device.dart';

const _goodScript = '(def x 0)\n'
    '@const-start\n'
    '(defun tick () { (setq x (+ x 1)) (sleep 1) })\n'
    '(spawn tick)\n'
    '@const-end\n';

// A defun outside the const block: acks fine on hardware, then OOMs.
const _badScript = '(defun stray () (print "x"))\n'
    '@const-start\n'
    '(defun tick () (sleep 1))\n'
    '@const-end\n'
    '(def y 0)\n'
    '(setq y 1)\n';

class _Harness {
  _Harness({
    bool writeEnabled = true,
    bool approve = true,
    int maxFlashes = 6,
    FakeLispDevice? device,
    String working = '',
  })  : dev = device ?? FakeLispDevice(),
        registry = ToolRegistry(writeEnabled: writeEnabled),
        _approve = approve {
    _working = working;
    ctx = ToolCtx(
      dev: dev,
      counters: AgentUsageCounters(),
      budget: AgentBudget(maxFlashes: maxFlashes),
      cancel: CancelToken(),
      confirm: (req) async {
        confirms.add(req);
        return _approve;
      },
      onProgress: (_) {},
      getWorking: () => _working,
      setWorking: (s) => _working = s,
    );
  }

  final FakeLispDevice dev;
  final ToolRegistry registry;
  final bool _approve;
  final confirms = <ConfirmRequest>[];
  late final ToolCtx ctx;
  late String _working;

  String get working => _working;

  Future<Map<String, dynamic>> call(String name,
      [Map<String, dynamic> args = const {}]) async {
    final spec = registry[name];
    if (spec == null) return {'ok': false, 'error': 'unknown_tool'};
    return spec.run(ctx, args, 'call_1');
  }
}

void main() {
  group('schema exposure', () {
    test('write tools are absent unless enabled', () {
      final names = (ToolRegistry(writeEnabled: false)
              .schemas(consoleAvailable: true))
          .map((s) => (s['function'] as Map)['name'])
          .toList();
      expect(names, isNot(contains('flash_script')));
      expect(names, isNot(contains('set_running')));
      expect(names, contains('apply_patch')); // editing is still fine
      expect(names, contains('lint_script'));
    });

    test('there is no tool that commands the motor directly', () {
      final names = ToolRegistry(writeEnabled: true)
          .schemas(consoleAvailable: true)
          .map((s) => (s['function'] as Map)['name'] as String)
          .toList();
      for (final forbidden in [
        'send_vesc_command',
        'conf_set',
        'set_current',
        'set_duty',
        'repl',
      ]) {
        expect(names, isNot(contains(forbidden)));
      }
    });

    test('a dead console channel removes the tool entirely', () {
      // An always-empty tool poisons the loop; better to not offer it.
      final names = ToolRegistry(writeEnabled: true)
          .schemas(consoleAvailable: false)
          .map((s) => (s['function'] as Map)['name'])
          .toList();
      expect(names, isNot(contains('read_console')));
    });

    test('strict mode marks each function', () {
      final s = ToolRegistry(writeEnabled: true)
          .schemas(consoleAvailable: true, strict: true)
          .first;
      expect((s['function'] as Map)['strict'], isTrue);
    });
  });

  group('reading', () {
    test('read_script returns metadata, not the text', () async {
      final h = _Harness(device: FakeLispDevice(initialCode: _goodScript));
      final r = await h.call('read_script');
      expect(r['ok'], isTrue);
      expect(r['loaded_from'], 'device');
      expect(r.containsKey('text'), isFalse); // read_lines is how you see it
      expect(h.working, _goodScript);
      expect(h.ctx.pristine, _goodScript);
    });

    test('read_lines carries the gutter', () async {
      final h = _Harness(working: 'a\nb\nc\n');
      final r = await h.call('read_lines', {'start': 2, 'count': 2});
      expect(r['text'], contains('   2| b'));
      expect(r['total_lines'], 4);
    });

    test('grep_script reports line numbers', () async {
      final h = _Harness(working: _goodScript);
      final r = await h.call('grep_script', {'pattern': 'spawn'});
      expect((r['matches'] as List).single['line'], 4);
    });

    test('a bad regex is an error the model can fix, not a crash', () async {
      final h = _Harness(working: 'x');
      final r = await h.call('grep_script', {'pattern': '(', 'is_regex': true});
      expect(r['ok'], isFalse);
      expect(r['error'], 'bad_regex');
    });

    test('get_stats reports the 18-binding truncation', () async {
      final dev = FakeLispDevice()..running = true;
      for (var i = 0; i < 20; i++) {
        dev.globals['g$i'] = i.toDouble();
      }
      final h = _Harness(device: dev);
      final r = await h.call('get_stats', {'samples': 1});
      expect(r['ok'], isTrue);
      expect(r['truncated_at_18'], isTrue);
    });

    test('device tools fail cleanly with no link', () async {
      final h = _Harness(device: FakeLispDevice()..connected = false);
      expect((await h.call('read_script'))['error'], 'no_link');
      expect((await h.call('get_stats'))['error'], 'no_link');
    });
  });

  group('editing', () {
    test('apply_patch edits the working copy and lints the result', () async {
      final h = _Harness(working: _goodScript);
      final r = await h.call('apply_patch', {
        'intent': 'slow the tick',
        'patch': '<<<<<<< SEARCH\n(sleep 1)\n=======\n(sleep 2)\n>>>>>>> REPLACE',
      });
      expect(r['ok'], isTrue);
      expect(h.working, contains('(sleep 2)'));
      expect(r['diff'], contains('+'));
      expect((r['lint'] as Map)['errors'], isEmpty);
    });

    test('a failed patch leaves the working copy untouched', () async {
      final h = _Harness(working: _goodScript);
      final r = await h.call('apply_patch',
          {'intent': 'x', 'patch': '<<<<<<< SEARCH\nnope\n=======\ny\n>>>>>>> REPLACE'});
      expect(r['ok'], isFalse);
      expect(h.working, _goodScript);
      expect(r['hint'], contains('byte-exactly'));
    });

    test('write_script demands a reason to replace real code', () async {
      final h = _Harness(working: _goodScript);
      final bad = await h.call('write_script', {'content': '(def z 1)', 'reason': ''});
      expect(bad['error'], 'needs_reason');
      expect(h.working, _goodScript);

      final ok = await h.call(
          'write_script', {'content': '(def z 1)', 'reason': 'total rewrite'});
      expect(ok['ok'], isTrue);
      expect(h.working, '(def z 1)');
    });

    test('revert_working_copy restores what was read from the device',
        () async {
      final h = _Harness(device: FakeLispDevice(initialCode: _goodScript));
      await h.call('read_script');
      await h.call('write_script',
          {'content': '(def broken 1)', 'reason': 'experiment'});
      expect(h.working, '(def broken 1)');
      final r = await h.call('revert_working_copy');
      expect(r['ok'], isTrue);
      expect(h.working, _goodScript);
    });
  });

  group('flashing', () {
    test('the linter is a hard gate', () async {
      final h = _Harness(working: _badScript);
      final r = await h.call('flash_script', {'run': true, 'rationale': 'go'});
      expect(r['ok'], isFalse);
      expect(r['error'], 'lint_blocked');
      expect(r['issues'], isNotEmpty);
      // Nothing was asked of the user and nothing reached the device.
      expect(h.confirms, isEmpty);
      expect(h.dev.calls, isEmpty);
    });

    test('every flash asks the user first', () async {
      final h = _Harness(working: _goodScript);
      await h.call('flash_script', {'run': false, 'rationale': 'bump'});
      expect(h.confirms, hasLength(1));
      expect(h.confirms.single.rationale, 'bump');
      expect(h.confirms.single.titleKey, 'agent.confirm.flash');
    });

    test('declining means nothing is written', () async {
      final h = _Harness(working: _goodScript, approve: false);
      final r = await h.call('flash_script', {'run': true, 'rationale': 'go'});
      expect(r['error'], 'user_declined');
      expect(h.dev.calls, isEmpty);
      expect(h.ctx.counters.flashes, 0);
    });

    test('an approved flash writes and verifies', () async {
      final dev = FakeLispDevice()..movingGlobals.add('x');
      final h = _Harness(working: _goodScript, device: dev);
      final r = await h.call('flash_script', {
        'run': true,
        'rationale': 'go',
        'moving_globals': ['x'],
      });
      expect(r['ok'], isTrue);
      expect(r['verdict'], 'ok');
      expect(dev.stored, _goodScript);
      expect(h.ctx.counters.flashes, 1);
    });

    test('reflashing identical bytes is refused', () async {
      final h = _Harness(working: _goodScript);
      await h.call('flash_script', {'run': false, 'rationale': 'first'});
      final again =
          await h.call('flash_script', {'run': false, 'rationale': 'again'});
      expect(again['error'], 'no_change');
    });

    test('the flash budget is enforced', () async {
      final h = _Harness(working: _goodScript, maxFlashes: 1);
      await h.call('flash_script', {'run': false, 'rationale': 'one'});
      h.ctx.lastFlashed = null; // pretend the content differs
      final r = await h.call('flash_script', {'run': false, 'rationale': 'two'});
      expect(r['error'], 'flash_budget');
    });

    test('a device failure comes back as a tool error, not an exception',
        () async {
      final dev = FakeLispDevice()..failNextUpload = StateError('erase failed');
      final h = _Harness(working: _goodScript, device: dev);
      final r = await h.call('flash_script', {'run': true, 'rationale': 'go'});
      expect(r['ok'], isFalse);
      expect(r['error'], 'flash_failed');
    });
  });

  group('set_running', () {
    test('stopping never asks — it is the safe direction', () async {
      final h = _Harness(device: FakeLispDevice()..running = true);
      final r = await h.call('set_running', {'run': false, 'reason': 'halt'});
      expect(r['ok'], isTrue);
      expect(h.confirms, isEmpty);
      expect(h.dev.running, isFalse);
    });

    test('starting asks', () async {
      final h = _Harness();
      await h.call('set_running', {'run': true, 'reason': 'test'});
      expect(h.confirms, hasLength(1));
      expect(h.dev.running, isTrue);
    });

    test('a declined start does not start anything', () async {
      final h = _Harness(approve: false);
      final r = await h.call('set_running', {'run': true, 'reason': 'test'});
      expect(r['error'], 'user_declined');
      expect(h.dev.running, isFalse);
    });
  });

  test('revert_to_flashed_backup restores the session-start script', () async {
    final dev = FakeLispDevice(initialCode: _goodScript);
    final h = _Harness(device: dev);
    await h.call('read_script');
    await h.call('write_script',
        {'content': '(def broken 1)', 'reason': 'experiment'});
    final r = await h.call('revert_to_flashed_backup', {'run': false});
    expect(r['ok'], isTrue);
    expect(dev.stored, _goodScript);
    expect(h.working, _goodScript);
  });

  test('get_link_info exposes the remaining flash budget', () async {
    final h = _Harness(maxFlashes: 3);
    final r = await h.call('get_link_info');
    expect(r['flashes_remaining'], 3);
    expect(r['lisp_max_bytes'], 120 * 1024);
  });
}
