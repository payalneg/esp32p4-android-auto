/// What the model is told before it touches a motor controller.
///
/// Two things are worth pinning: the prompt carries the rules whose violation
/// fails silently on hardware, and the API reference does not invent builtins —
/// every call it teaches must be one the script or the upstream docs use.
library;

import 'dart:io';

import 'package:aa_bridge/agent/agent_prompt.dart';
import 'package:aa_bridge/agent/lisp_reference.dart';
import 'package:flutter_test/flutter_test.dart';

String? _script() {
  for (final p in [
    '/Users/alexey/esp32c3-ble-helper/lisp/main.lisp',
    '../lisp/main.lisp',
    'lisp/main.lisp',
  ]) {
    final f = File(p);
    if (f.existsSync()) return f.readAsStringSync();
  }
  return null;
}

void main() {
  group('system prompt', () {
    test('carries the rules that fail silently on hardware', () {
      for (final needle in [
        '@const-start',
        'out_of_memory',
        '18 bindings',
        'moving_globals',
        '120 KiB',
        'read_lines',
      ]) {
        expect(kAgentSystemPrompt, contains(needle), reason: needle);
      }
    });

    test('states the forward-reference rule, not "spawns last"', () {
      // main.lisp legitimately spawns from the middle of its const block, so
      // the prompt must teach the real invariant.
      expect(kAgentSystemPrompt, contains('No forward references'));
      expect(kAgentSystemPrompt, contains('spawning from the middle'));
    });

    test('includes the API reference', () {
      expect(kAgentSystemPrompt, contains(kLispReference));
      expect(kAgentSystemPrompt.length, greaterThan(8000));
    });

    test('is a compile-time constant, so the cache prefix cannot drift', () {
      const a = kAgentSystemPrompt;
      const b = kAgentSystemPrompt;
      expect(identical(a, b), isTrue);
    });
  });

  group('API reference', () {
    test('documents the calls this project actually depends on', () {
      for (final fn in [
        'set-current-rel',
        'get-adc-decoded',
        'app-disable-output',
        'throttle-curve',
        'conf-get',
        'conf-set',
        'foc-play-tone',
        'eeprom-store-i',
        'bufcreate',
        'send-data',
        'event-can-sid',
        'secs-since',
      ]) {
        expect(kLispReference, contains(fn), reason: fn);
      }
    });

    test('teaches only builtins the running script also uses', () {
      final script = _script();
      if (script == null) return;
      // Sample the calls the reference presents as VESC extensions and check
      // they appear in real, working code.
      for (final fn in [
        'get-rpm',
        'set-current',
        'set-brake-rel',
        'gpio-read',
        'shutdown-hold',
        'bufget-i32',
        'str-merge',
        'to-str',
      ]) {
        expect(script, contains(fn), reason: '$fn should exist in main.lisp');
        expect(kLispReference, contains(fn), reason: fn);
      }
    });

    test('the worked examples respect the @const rules they preach', () {
      // The debug-counter example is the one the model will copy most often.
      final example = kLispReference
          .substring(kLispReference.indexOf('(def dbg-tick 0)'));
      expect(example.indexOf('(def dbg-tick 0)'),
          lessThan(example.indexOf('@const-start')));
      expect(example.indexOf('(defun dbg-loop'),
          lessThan(example.indexOf('(spawn 150 dbg-loop)')));
    });

    test('flags the two irreversible-looking traps', () {
      expect(kLispReference, contains('flashed buffer is read-only'));
      expect(kLispReference, contains('not on a bench reboot'));
    });
  });
}
