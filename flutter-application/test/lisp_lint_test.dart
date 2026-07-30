/// The golden fixture here is the real `lisp/main.lisp` — it MUST lint clean.
///
/// That test is the whole defence against a false-positive-happy linter: the
/// linter gates flashing, so if it cries wolf on the reference script users
/// will switch the gate off and the real checks go with it. The mutants below
/// then prove each rule still fires when it should.
library;

import 'dart:io';

import 'package:aa_bridge/agent/lisp_lint.dart';
import 'package:flutter_test/flutter_test.dart';

String get _mainLisp {
  // Tests run with CWD = flutter-application/.
  for (final p in ['../lisp/main.lisp', 'lisp/main.lisp']) {
    final f = File(p);
    if (f.existsSync()) return f.readAsStringSync();
  }
  fail('lisp/main.lisp not found relative to ${Directory.current.path}');
}

Set<String> _codes(LintReport r, [LintLevel? level]) => {
      for (final i in r.issues)
        if (level == null || i.level == level) i.code
    };

void main() {
  group('golden: lisp/main.lisp', () {
    late LintReport report;

    setUpAll(() => report = lintLisp(_mainLisp));

    test('lints with zero errors', () {
      expect(report.hasErrors, isFalse,
          reason: 'errors: ${report.errors.join('\n')}');
    });

    test('finds the @const block', () {
      expect(report.constStartLine, isNotNull);
      expect(report.constEndLine, isNotNull);
      expect(report.constEndLine!, greaterThan(report.constStartLine!));
    });

    test('does not flag the mid-const spawns as use-before-bind', () {
      // main.lisp:225-227 spawns three threads from inside the const block,
      // and its final spawn/event block sits inside @const-start…@const-end
      // too. The rule is "no forward references", not "spawns last".
      expect(_codes(report), isNot(contains('E_USE_BEFORE_BIND')));
    });

    test('notes the 18-binding stats cap', () {
      expect(report.topLevelDefs, greaterThan(18));
      expect(_codes(report, LintLevel.info), contains('I_BINDING_CAP'));
    });

    test('flags the melody literal as a load-time heap cost', () {
      expect(_codes(report, LintLevel.warn), contains('W_LARGE_QUOTED'));
    });

    test('packed size is well under the flash limit', () {
      expect(report.packedBytes, greaterThan(20000));
      expect(report.packedBytes, lessThan(kLispMaxBytes));
    });
  });

  group('mutants of the golden script', () {
    // NB: the literal text "@const-start" also appears in the comment that
    // explains it (main.lisp:45), so mutants anchor on "\n@const-start" —
    // only the real marker starts a line.
    String belowConst(String src, String inject) =>
        src.replaceFirst('\n@const-start', '\n@const-start\n$inject');

    test('a setq\'d def moved below @const-start', () {
      final src = belowConst(
          _mainLisp.replaceFirst('(def out-rel 0.0)', ''), '(def out-rel 0.0)');
      final r = lintLisp(src);
      expect(_codes(r, LintLevel.error), contains('E_MUTABLE_BELOW_CONST'));
    });

    test('a bufcreate moved below @const-start', () {
      final src = belowConst(
          _mainLisp.replaceFirst('(def pbuf (bufcreate 128))', ''),
          '(def pbuf (bufcreate 128))');
      final r = lintLisp(src);
      expect(_codes(r, LintLevel.error), contains('E_BUFCREATE_BELOW_CONST'));
    });

    test('a spawn hoisted above its target defun', () {
      final src = belowConst(
          _mainLisp.replaceFirst('(spawn 150 monitor-rx-button)\n', ''),
          '(spawn 150 monitor-rx-button)');
      final r = lintLisp(src);
      expect(_codes(r, LintLevel.error), contains('E_USE_BEFORE_BIND'));
    });

    test('a defun left above @const-start is warned about', () {
      final src = _mainLisp.replaceFirst(
          '\n@const-start', '\n(defun stray () (print "x"))\n@const-start');
      final r = lintLisp(src);
      expect(_codes(r, LintLevel.warn), contains('W_DEFUN_ABOVE_CONST'));
    });

    test('an unbalanced paren', () {
      final r = lintLisp('$_mainLisp\n(defun oops () { (print "x") ');
      expect(_codes(r, LintLevel.error), contains('E_PARENS'));
    });

    test('@const-start with no @const-end', () {
      final r = lintLisp(_mainLisp.replaceFirst('@const-end', ''));
      expect(_codes(r, LintLevel.error), contains('E_CONST_PAIR'));
    });
  });

  group('small scripts', () {
    test('a minimal well-formed script is clean', () {
      final r = lintLisp('(def x 0)\n'
          '@const-start\n'
          '(defun tick () { (setq x (+ x 1)) (sleep 1) })\n'
          '(spawn tick)\n'
          '@const-end\n');
      expect(r.hasErrors, isFalse, reason: r.errors.join('\n'));
    });

    test('brace-progn is not mistaken for an unbalanced paren', () {
      final r = lintLisp('(defun f () { (if t { (print "a") } { (print "b") }) })');
      expect(_codes(r, LintLevel.error), isNot(contains('E_PARENS')));
    });

    test('a brace closed by a paren is an error', () {
      final r = lintLisp('(defun f () { (print "x") ) )');
      expect(_codes(r, LintLevel.error), contains('E_PARENS'));
    });

    test('parens inside comments and strings are ignored', () {
      final r = lintLisp('; ))) not code\n'
          '(def s "a ) string ( with parens")\n');
      expect(_codes(r, LintLevel.error), isNot(contains('E_PARENS')));
    });

    test('quoted symbols do not count as references', () {
      // 'event-data-rx must not be read as a use of a later binding.
      final r = lintLisp("(event-enable 'event-data-rx)\n"
          '(def event-data-rx 1)\n');
      expect(_codes(r, LintLevel.error), isNot(contains('E_USE_BEFORE_BIND')));
    });

    test('event-enable without a handler is a warning', () {
      final r = lintLisp("(event-enable 'event-shutdown)");
      expect(_codes(r, LintLevel.warn), contains('W_EVENT_MISMATCH'));
    });

    test('an oversized script is rejected', () {
      final r = lintLisp('(def big "${'x' * (121 * 1024)}")');
      expect(_codes(r, LintLevel.error), contains('E_TOO_BIG'));
    });
  });
}
