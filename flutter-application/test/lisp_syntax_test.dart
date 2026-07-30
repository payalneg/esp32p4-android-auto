/// Highlighter tests.
///
/// It runs on every keystroke over a 23 KB script, so the two properties that
/// matter are: it never loses or reorders a character (the TextField would
/// show something different from what you typed), and it never throws on
/// half-written code.
library;

import 'package:aa_bridge/ui/lisp_syntax.dart';
import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';

const _style = TextStyle(fontFamily: 'monospace');

const _theme = LispTheme(
  base: Color(0xFF000001),
  comment: Color(0xFF000002),
  string: Color(0xFF000003),
  number: Color(0xFF000004),
  keyword: Color(0xFF000005),
  constMarker: Color(0xFF000006),
  quote: Color(0xFF000007),
  parens: [Color(0xFF000010), Color(0xFF000011)],
);

String plain(TextSpan span) {
  final b = StringBuffer();
  void walk(InlineSpan s) {
    if (s is TextSpan) {
      if (s.text != null) b.write(s.text);
      for (final c in s.children ?? const <InlineSpan>[]) {
        walk(c);
      }
    }
  }

  walk(span);
  return b.toString();
}

/// Colour of the first span whose text equals [needle].
Color? colorOf(TextSpan root, String needle) {
  Color? found;
  void walk(InlineSpan s) {
    if (found != null) return;
    if (s is TextSpan) {
      if (s.text == needle) {
        found = s.style?.color;
        return;
      }
      for (final c in s.children ?? const <InlineSpan>[]) {
        walk(c);
      }
    }
  }

  walk(root);
  return found;
}

void main() {
  group('round-trip', () {
    test('reproduces the source exactly', () {
      const src = '(defun tick () {\n'
          '  ; bump it\n'
          '  (setq x (+ x 1.5))\n'
          '  (print "hi \\"there\\"")\n'
          '})\n';
      expect(plain(highlightLisp(src, _style, _theme)), src);
    });

    test('preserves whitespace, tabs and blank lines', () {
      const src = '(a)\n\n\t(b)   \n   ';
      expect(plain(highlightLisp(src, _style, _theme)), src);
    });

    test('empty input is fine', () {
      expect(plain(highlightLisp('', _style, _theme)), '');
    });
  });

  group('colouring', () {
    test('keywords, numbers, strings and comments differ from base', () {
      final s = highlightLisp(
          '(defun f () (setq x 42))\n; note\n(print "text")',
          _style,
          _theme);
      expect(colorOf(s, 'defun'), _theme.keyword);
      expect(colorOf(s, 'setq'), _theme.keyword);
      expect(colorOf(s, '42'), _theme.number);
      expect(colorOf(s, '; note'), _theme.comment);
      expect(colorOf(s, '"text"'), _theme.string);
      expect(colorOf(s, 'print'), _theme.base); // builtins stay plain
    });

    test('@const markers get their own colour and weight', () {
      final s = highlightLisp('@const-start\n(def x 1)\n@const-end',
          _style, _theme);
      expect(colorOf(s, '@const-start'), _theme.constMarker);
      expect(colorOf(s, '@const-end'), _theme.constMarker);
    });

    test('parens cycle by depth so pairs match visually', () {
      final s = highlightLisp('((a))', _style, _theme);
      final spans = <TextSpan>[];
      void walk(InlineSpan x) {
        if (x is TextSpan) {
          if (x.text != null) spans.add(x);
          for (final c in x.children ?? const <InlineSpan>[]) {
            walk(c);
          }
        }
      }

      walk(s);
      final brackets =
          spans.where((x) => x.text == '(' || x.text == ')').toList();
      expect(brackets, hasLength(4));
      // outer ( and its ) share a colour; the inner pair uses the next one
      expect(brackets[0].style?.color, brackets[3].style?.color);
      expect(brackets[1].style?.color, brackets[2].style?.color);
      expect(brackets[0].style?.color, isNot(brackets[1].style?.color));
    });

    test('braces nest with parens (LispBM progn sugar)', () {
      final s = highlightLisp('({x})', _style, _theme);
      expect(plain(s), '({x})');
      expect(colorOf(s, '{'), isNot(colorOf(s, '(')));
    });

    test('numbers with LispBM suffixes are numbers', () {
      final s = highlightLisp('(0x1F 3.5 -2 10u32)', _style, _theme);
      for (final n in ['0x1F', '3.5', '-2', '10u32']) {
        expect(colorOf(s, n), _theme.number, reason: n);
      }
    });

    test('parens inside strings and comments are not counted', () {
      // If they were, everything after would be coloured at the wrong depth.
      final s = highlightLisp('(a ") ((" ; ))) \n b)', _style, _theme);
      expect(plain(s), '(a ") ((" ; ))) \n b)');
      expect(colorOf(s, '") (("'), _theme.string);
    });
  });

  group('unfinished input', () {
    test('an unterminated string does not throw', () {
      expect(() => highlightLisp('(print "abc', _style, _theme),
          returnsNormally);
      expect(plain(highlightLisp('(print "abc', _style, _theme)),
          '(print "abc');
    });

    test('a trailing backslash inside a string does not throw', () {
      expect(() => highlightLisp(r'"abc\', _style, _theme), returnsNormally);
    });

    test('unbalanced closers keep depth non-negative', () {
      expect(() => highlightLisp(')))', _style, _theme), returnsNormally);
      expect(plain(highlightLisp(')))', _style, _theme)), ')))');
    });

    test('a lone quote is fine', () {
      expect(plain(highlightLisp("'", _style, _theme)), "'");
    });
  });

  test('a realistic script is fast enough for per-keystroke use', () {
    final src = List.generate(
        400,
        (i) => '(defun f$i (x) { (setq acc (+ acc $i)) '
            '(print "step $i") }) ; line $i').join('\n');
    final sw = Stopwatch()..start();
    highlightLisp(src, _style, _theme);
    sw.stop();
    expect(plain(highlightLisp(src, _style, _theme)), src);
    expect(sw.elapsedMilliseconds, lessThan(120));
  });
}
