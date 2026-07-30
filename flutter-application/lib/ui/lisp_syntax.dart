/// Syntax colouring for LispBM.
///
/// Hand-rolled rather than pulling in a highlighting package: the language is
/// small, the rules fit on a screen, and this way the two things that actually
/// bite on this project get their own colour — `@const-start` / `@const-end`
/// (put a defun on the wrong side of them and the script dies at runtime with
/// the acks still saying OK) and paren depth, which is how you find the end of
/// a 40-line `defun` on a phone screen.
///
/// Used both by the editor's [TextField] (via [LispEditingController]) and by
/// the assistant's code blocks, so what the model shows you and what you edit
/// look the same.
library;

import 'package:flutter/material.dart';

/// Structural forms. Deliberately NOT a list of every builtin: colouring a
/// couple of hundred function names turns the screen into confetti and hides
/// the structure, which is the thing worth seeing.
const _keywords = {
  'def', 'defun', 'defunret', 'define', 'setq', 'setvar', 'let', 'lambda',
  'if', 'cond', 'match', 'progn', 'atomic', 'and', 'or', 'not',
  'loopwhile', 'loopfor', 'loopwhile-thd', 'looprange', 'recv', 'recv-to',
  'spawn', 'spawn-trap', 'event-register-handler', 'event-enable', 'yield',
};

class LispTheme {
  const LispTheme({
    required this.base,
    required this.comment,
    required this.string,
    required this.number,
    required this.keyword,
    required this.constMarker,
    required this.quote,
    required this.parens,
  });

  final Color base;
  final Color comment;
  final Color string;
  final Color number;
  final Color keyword;

  /// `@const-start` / `@const-end` — the line everything hinges on.
  final Color constMarker;
  final Color quote;

  /// Cycled by nesting depth, so a closing paren visibly pairs up.
  final List<Color> parens;

  factory LispTheme.of(BuildContext context) {
    final scheme = Theme.of(context).colorScheme;
    final dark = Theme.of(context).brightness == Brightness.dark;
    return LispTheme(
      base: scheme.onSurface,
      comment: scheme.outline,
      string: dark ? const Color(0xFF7EC699) : const Color(0xFF2E7D32),
      number: dark ? const Color(0xFFE5A663) : const Color(0xFFB35C00),
      keyword: dark ? const Color(0xFF9BB4FF) : const Color(0xFF2F4FCF),
      constMarker: dark ? const Color(0xFFFF8A80) : const Color(0xFFC62828),
      quote: dark ? const Color(0xFFC792EA) : const Color(0xFF6A1B9A),
      parens: dark
          ? const [Color(0xFFB0BEC5), Color(0xFF80CBC4), Color(0xFFCE93D8),
              Color(0xFFFFCC80)]
          : const [Color(0xFF546E7A), Color(0xFF00796B), Color(0xFF6A1B9A),
              Color(0xFFEF6C00)],
    );
  }
}

/// Colour [source] as LispBM.
///
/// Tolerant of half-typed code by construction — an unterminated string or a
/// missing paren just colours to the end of the buffer rather than throwing,
/// because this runs on every keystroke.
TextSpan highlightLisp(String source, TextStyle style, LispTheme theme) {
  final spans = <TextSpan>[];
  final buf = StringBuffer();
  var depth = 0;

  void flush([Color? color]) {
    if (buf.isEmpty) return;
    spans.add(TextSpan(
        text: buf.toString(), style: style.copyWith(color: color ?? theme.base)));
    buf.clear();
  }

  void emit(String text, Color color, {FontWeight? weight}) {
    spans.add(TextSpan(
        text: text,
        style: style.copyWith(color: color, fontWeight: weight)));
  }

  var i = 0;
  while (i < source.length) {
    final c = source[i];

    // Line comment.
    if (c == ';') {
      flush();
      final start = i;
      while (i < source.length && source[i] != '\n') {
        i++;
      }
      emit(source.substring(start, i), theme.comment);
      continue;
    }

    // String, with backslash escapes.
    if (c == '"') {
      flush();
      final start = i;
      i++;
      while (i < source.length) {
        if (source[i] == r'\') {
          i += 2;
          continue;
        }
        if (source[i] == '"') {
          i++;
          break;
        }
        i++;
      }
      emit(source.substring(start, i.clamp(0, source.length)), theme.string);
      continue;
    }

    // Brackets: `{}` is LispBM's progn sugar and nests with `()`.
    if (c == '(' || c == '{') {
      flush();
      emit(c, theme.parens[depth % theme.parens.length]);
      depth++;
      i++;
      continue;
    }
    if (c == ')' || c == '}') {
      flush();
      depth = depth > 0 ? depth - 1 : 0;
      emit(c, theme.parens[depth % theme.parens.length]);
      i++;
      continue;
    }

    if (c == "'") {
      flush();
      emit(c, theme.quote);
      i++;
      continue;
    }

    // Atom.
    if (!_isDelimiter(c) && !_isSpace(c)) {
      flush();
      final start = i;
      while (i < source.length &&
          !_isDelimiter(source[i]) &&
          !_isSpace(source[i])) {
        i++;
      }
      final word = source.substring(start, i);
      if (word.startsWith('@const')) {
        emit(word, theme.constMarker, weight: FontWeight.bold);
      } else if (_keywords.contains(word)) {
        emit(word, theme.keyword, weight: FontWeight.w600);
      } else if (_isNumber(word)) {
        emit(word, theme.number);
      } else {
        emit(word, theme.base);
      }
      continue;
    }

    buf.write(c);
    i++;
  }
  flush();

  return TextSpan(style: style, children: spans);
}

bool _isDelimiter(String c) =>
    c == '(' || c == ')' || c == '{' || c == '}' || c == ';' || c == '"' ||
    c == "'";

bool _isSpace(String c) => c == ' ' || c == '\t' || c == '\n' || c == '\r';

bool _isNumber(String s) {
  if (s.isEmpty) return false;
  if (double.tryParse(s) != null) return true;
  // 0x1F / 1u32 / 3.5f — LispBM's numeric suffixes.
  return RegExp(r'^-?(0x[0-9a-fA-F]+|[0-9]+\.?[0-9]*)(u28|i28|u32|i32|u64|i64|f32|f64|b)?$')
      .hasMatch(s);
}

/// A [TextEditingController] that paints its own text.
///
/// The span is cached per (text, theme): `buildTextSpan` runs on every frame
/// while the caret blinks, and re-tokenising a 23 KB script that often is
/// visible jank on a mid-range phone.
class LispEditingController extends TextEditingController {
  LispEditingController({super.text});

  String? _cachedFor;
  LispTheme? _cachedTheme;
  TextStyle? _cachedStyle;
  TextSpan? _cached;

  @override
  TextSpan buildTextSpan({
    required BuildContext context,
    TextStyle? style,
    required bool withComposing,
  }) {
    final theme = LispTheme.of(context);
    final base = style ?? const TextStyle();
    if (_cached != null &&
        _cachedFor == text &&
        _cachedTheme?.base == theme.base &&
        _cachedStyle == base) {
      return _cached!;
    }
    final span = highlightLisp(text, base, theme);
    _cachedFor = text;
    _cachedTheme = theme;
    _cachedStyle = base;
    _cached = span;
    return span;
  }
}

/// Read-only coloured code, for the assistant's fenced blocks.
class LispCodeView extends StatelessWidget {
  const LispCodeView(this.code, {super.key, this.fontSize = 12});
  final String code;
  final double fontSize;

  @override
  Widget build(BuildContext context) {
    final style = TextStyle(
        fontFamily: 'monospace', fontSize: fontSize, height: 1.35);
    return SelectableText.rich(
      highlightLisp(code, style, LispTheme.of(context)),
      style: style,
    );
  }
}
