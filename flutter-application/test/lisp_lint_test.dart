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

    test('has no large quoted literal left to warn about', () {
      // The script used to carry a ~2 kB melody list; the tune controls are
      // gone, so the golden is expected clean here. W_LARGE_QUOTED itself is
      // covered by the synthetic literal in 'small scripts' below.
      expect(_codes(report, LintLevel.warn), isNot(contains('W_LARGE_QUOTED')));
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

    test('a large quoted literal is flagged as a load-time heap cost', () {
      // >512 B of quoted list: the reader builds the whole thing on the cons
      // heap before @const can flash it, so it can OOM at load time.
      final notes = List.filled(60, '(330 0.124)').join(' ');
      final r = lintLisp("(def tune '($notes))\n");
      expect(_codes(r, LintLevel.warn), contains('W_LARGE_QUOTED'));
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

  // The panel frames are decoded byte-for-byte by vesc_lisp_panel.c and every
  // deviation is dropped silently on the P4 — no error reaches the phone, the
  // row simply isn't drawn. These checks are the only thing standing between
  // the model and a change that flashes, verifies and does nothing.
  group('quick-action panel', () {
    /// A minimal script shaped like main.lisp's panel: mutable buffer above
    /// the const block, the three append helpers, then the senders.
    String panel({
      required String ui,
      String state = '',
      String action = '',
      int buf = 128,
      int uiCount = 1,
      int stateCount = 0,
      String extra = '',
    }) =>
        '(def pbuf (bufcreate $buf))\n'
        '(def pi 0)\n'
        '@const-start\n'
        '(defun pu8  (v) { (bufset-u8  pbuf pi v) (setq pi (+ pi 1)) })\n'
        '(defun pi32 (v) { (bufset-i32 pbuf pi v) (setq pi (+ pi 4)) })\n'
        '(defun pstr (s) { (bufcpy pbuf pi s 0 (buflen s)) '
        '(setq pi (+ pi (buflen s))) })\n'
        '(defun panel-send-ui (reply-id) {\n'
        '    (setq pi 0)\n'
        '    (pu8 0x56) (pu8 0x50) (pu8 0x81) (pu8 1) (pu8 $uiCount)\n'
        '$ui'
        '    (send-data pbuf 2 reply-id)\n'
        '})\n'
        '(defun panel-send-state (reply-id) {\n'
        '    (setq pi 0)\n'
        '    (pu8 0x56) (pu8 0x50) (pu8 0x82) (pu8 $stateCount)\n'
        '$state'
        '    (send-data pbuf 2 reply-id)\n'
        '})\n'
        '(defun panel-action (cid val) {\n'
        '    (cond\n'
        '$action'
        '        (t nil))\n'
        '})\n'
        '$extra'
        '@const-end\n';

    Set<String> panelCodes(LintReport r) =>
        {for (final i in r.issues) if (i.code.contains('PANEL')) i.code};

    test('the golden panel raises nothing', () {
      expect(panelCodes(lintLisp(_mainLisp)), isEmpty);
    });

    test('the live id/type swap is rejected', () {
      // Verbatim what the model produced: <id> and <type> the wrong way round,
      // so the type byte reads 8 and the P4 truncates the rest of the frame.
      final r = lintLisp(panel(ui: '    (pu8 1) (pu8 8) (pstr "Profile") '
          '(pu8 1)\n'));
      expect(_codes(r, LintLevel.error), contains('E_PANEL_CTRL_TYPE'));
      final issue =
          r.errors.firstWhere((i) => i.code == 'E_PANEL_CTRL_TYPE');
      expect(issue.hint, contains('swapped'));
    });

    test('a plausible but unknown type is rejected without a swap hint', () {
      final r = lintLisp(panel(ui: '    (pu8 9) (pu8 7) (pstr "X")\n'));
      final issue =
          r.errors.firstWhere((i) => i.code == 'E_PANEL_CTRL_TYPE');
      expect(issue.hint, isNot(contains('swapped')));
    });

    test('a UI count byte that disagrees is rejected', () {
      final r = lintLisp(panel(
        uiCount: 3,
        ui: '    (pu8 1) (pu8 2) (pstr "A")\n    (pu8 2) (pu8 2) (pstr "B")\n',
        action: '        ((= cid 1) (a))\n        ((= cid 2) (b))\n',
      ));
      expect(_codes(r, LintLevel.error), contains('E_PANEL_COUNT'));
    });

    test('a STATE count byte that disagrees is rejected', () {
      final r = lintLisp(panel(
        ui: '    (pu8 1) (pu8 1) (pstr "A") (pu8 x)\n',
        stateCount: 2,
        state: '    (pu8 1) (pi32 (* x 1000))\n',
      ));
      expect(_codes(r, LintLevel.error), contains('E_PANEL_COUNT'));
    });

    test('a duplicate control id is rejected', () {
      final r = lintLisp(panel(
        uiCount: 2,
        ui: '    (pu8 7) (pu8 2) (pstr "A")\n    (pu8 7) (pu8 2) (pstr "B")\n',
        action: '        ((= cid 7) (a))\n',
      ));
      expect(_codes(r, LintLevel.error), contains('E_PANEL_DUP_ID'));
    });

    test('a STATE entry for an undeclared id is rejected', () {
      final r = lintLisp(panel(
        ui: '    (pu8 1) (pu8 1) (pstr "A") (pu8 x)\n',
        stateCount: 2,
        state: '    (pu8 1) (pi32 (* x 1000))\n    (pu8 9) (pi32 0)\n',
      ));
      expect(_codes(r, LintLevel.error), contains('E_PANEL_STATE_ID'));
    });

    test('an action branch for an undeclared id is rejected', () {
      final r = lintLisp(panel(
        uiCount: 2,
        ui: '    (pu8 1) (pu8 2) (pstr "A")\n    (pu8 2) (pu8 2) (pstr "B")\n',
        action: '        ((= cid 1) (a))\n        ((= cid 8) (b))\n',
      ));
      expect(_codes(r, LintLevel.error), contains('E_PANEL_ACTION_ID'));
    });

    test('a control with neither state nor action is flagged', () {
      final r = lintLisp(panel(
        uiCount: 2,
        ui: '    (pu8 1) (pu8 2) (pstr "A")\n    (pu8 2) (pu8 2) (pstr "B")\n',
        action: '        ((= cid 1) (a))\n',
      ));
      expect(_codes(r, LintLevel.info), contains('I_PANEL_CTRL_INERT'));
    });

    test('a read-only label needs neither state nor action', () {
      final r = lintLisp(panel(
        uiCount: 2,
        ui: '    (pu8 1) (pu8 2) (pstr "A")\n'
            '    (pu8 2) (pu8 4) (pstr "Temp") (pi32 (* (get-temp-mot) 1000)) '
            '(pstr "C")\n',
        action: '        ((= cid 1) (a))\n',
      ));
      expect(_codes(r, LintLevel.info), isNot(contains('I_PANEL_CTRL_INERT')));
    });

    test('an over-long label is rejected, 39 bytes is not', () {
      final long = lintLisp(panel(
          buf: 512, ui: '    (pu8 1) (pu8 2) (pstr "${'x' * 40}")\n'));
      expect(_codes(long, LintLevel.error), contains('E_PANEL_STR_LEN'));
      final ok = lintLisp(panel(
          buf: 512, ui: '    (pu8 1) (pu8 2) (pstr "${'x' * 39}")\n'));
      expect(_codes(ok, LintLevel.error), isNot(contains('E_PANEL_STR_LEN')));
    });

    test('an over-long unit suffix is rejected', () {
      final r = lintLisp(panel(
        buf: 512,
        ui: '    (pu8 1) (pu8 3) (pstr "N") (pi32 0) (pi32 1000) (pi32 1000) '
            '(pi32 0) (pstr "${'u' * 12}")\n',
      ));
      expect(_codes(r, LintLevel.error), contains('E_PANEL_STR_LEN'));
    });

    test('a frame wider than its buffer is rejected', () {
      final r = lintLisp(panel(
          buf: 8, ui: '    (pu8 1) (pu8 2) (pstr "A long enough label")\n'));
      expect(_codes(r, LintLevel.error), contains('E_PANEL_BUF_OVERFLOW'));
    });

    test('seventeen controls warn', () {
      final rows = [
        for (var i = 1; i <= 17; i++) '    (pu8 $i) (pu8 2) (pstr "B$i")\n'
      ].join();
      final acts = [for (var i = 1; i <= 17; i++) '        ((= cid $i) (a))\n']
          .join();
      final r = lintLisp(
          panel(buf: 512, uiCount: 17, ui: rows, action: acts));
      expect(_codes(r, LintLevel.warn), contains('W_PANEL_TOO_MANY'));
    });

    // Everything below asserts SILENCE. A linter that gates flashing must
    // never guess: an unrecognised shape has to produce no diagnostic at all.

    test('a script with no panel gets no panel diagnostics', () {
      final r = lintLisp('(def x 0)\n'
          '@const-start\n'
          '(defun tick () { (setq x (+ x 1)) (sleep 1) })\n'
          '(spawn tick)\n'
          '@const-end\n');
      expect(panelCodes(r), isEmpty);
    });

    test("a number control's empty suffix is not read as the next label", () {
      // The (pstr "") tail is consumed BY TYPE, so it can never start a row.
      final r = lintLisp(panel(
        uiCount: 2,
        ui: '    (pu8 5) (pu8 3) (pstr "Vol") (pi32 0) (pi32 50000) '
            '(pi32 5000) (pi32 (* v 1000)) (pstr "")\n'
            '    (pu8 6) (pu8 2) (pstr "Beep")\n',
        stateCount: 1,
        state: '    (pu8 5) (pi32 (* v 1000))\n',
        action: '        ((= cid 6) (beep))\n',
      ));
      expect(panelCodes(r), isEmpty);
    });

    test('a conditionally emitted row silences the count check', () {
      final r = lintLisp(panel(
        uiCount: 9,
        ui: '    (if tc-on { (pu8 2) (pu8 2) (pstr "TC") })\n',
      ));
      expect(panelCodes(r), isEmpty);
    });

    test('a row emitted by a helper call silences the count check', () {
      final r = lintLisp(panel(
        uiCount: 9,
        ui: '    (pu8 1) (pu8 2) (pstr "A")\n'
            '    (emit-row 2 "B")\n'
            '    (pu8 3) (pu8 2) (pstr "C")\n',
      ));
      expect(panelCodes(r), isEmpty);
    });

    test('a computed count byte is not checked', () {
      final r = lintLisp(
        '(def pbuf (bufcreate 128))\n(def pi 0)\n(def n-ctrls 2)\n'
        '@const-start\n'
        '(defun pu8 (v) { (bufset-u8 pbuf pi v) (setq pi (+ pi 1)) })\n'
        '(defun pstr (s) { (bufcpy pbuf pi s 0 (buflen s)) })\n'
        '(defun panel-send-ui (reply-id) {\n'
        '    (setq pi 0)\n'
        '    (pu8 0x56) (pu8 0x50) (pu8 0x81) (pu8 1) (pu8 n-ctrls)\n'
        '    (pu8 1) (pu8 2) (pstr "A")\n'
        '    (send-data pbuf 2 reply-id)\n'
        '})\n'
        '@const-end\n',
      );
      expect(panelCodes(r), isEmpty);
    });

    test('a computed type byte is not guessed at', () {
      final r = lintLisp(panel(ui: '    (pu8 1) (pu8 ctype) (pstr "A")\n'));
      expect(panelCodes(r), isEmpty);
    });

    test('a computed label silences the size check', () {
      final r = lintLisp(panel(
          buf: 8,
          ui: '    (pu8 1) (pu8 2) (pstr (str-merge "A" suffix))\n'));
      expect(panelCodes(r), isEmpty);
    });

    test('a 0x84 dash frame is not parsed as a panel', () {
      final r = lintLisp(
        '(def pbuf (bufcreate 128))\n(def pi 0)\n'
        '@const-start\n'
        '(defun pu8 (v) { (bufset-u8 pbuf pi v) (setq pi (+ pi 1)) })\n'
        '(defun pi32 (v) { (bufset-i32 pbuf pi v) (setq pi (+ pi 4)) })\n'
        '(defun panel-send-dash (reply-id) {\n'
        '    (setq pi 0)\n'
        '    (pu8 0x56) (pu8 0x50) (pu8 0x84)\n'
        '    (pi32 1) (pi32 2) (pi32 3) (pi32 4)\n'
        '    (send-data pbuf 2 reply-id)\n'
        '})\n'
        '@const-end\n',
      );
      expect(panelCodes(r), isEmpty);
    });

    test('cid meaning a CAN id is not read as a control id', () {
      final r = lintLisp(panel(
        ui: '    (pu8 1) (pu8 2) (pstr "A")\n',
        action: '        ((= cid 1) (a))\n',
        extra: '(defun on-frame (cid data) '
            '(cond ((= cid 42) (print "x")) (t nil)))\n',
      ));
      expect(panelCodes(r), isEmpty);
    });

    test('broken parens skip the panel checks entirely', () {
      final r = lintLisp('${panel(uiCount: 9, ui: '    (pu8 1) (pu8 2) '
          '(pstr "A")\n')}\n(defun oops () { (print "x") ');
      expect(_codes(r, LintLevel.error), contains('E_PARENS'));
      expect(panelCodes(r), isEmpty);
    });
  });
}
