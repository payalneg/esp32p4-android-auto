/// Structural checks for a VESC LispBM script.
///
/// This encodes the project's hard-won authoring rules — the ones that
/// currently live only in `lisp/main.lisp`'s comments and in commit messages —
/// because every one of them fails SILENTLY on hardware: the erase/write/run
/// acks all report success and the script is simply dead, or half-dead with
/// the motor still under its control.
///
///   * All `defun`s belong inside ONE `@const-start … @const-end` block. Left
///     outside, their bodies live in the cons heap, exhaust it, and the event
///     loop dies at runtime with `out_of_memory`.
///   * Mutable state (`def`s that get `setq`'d) and `bufcreate` buffers MUST
///     stay ABOVE `@const-start` — a flashed buffer is read-only.
///   * A top-level statement must not reference a symbol bound LATER in the
///     file. This is the real invariant behind the "spawn threads last" rule:
///     `main.lisp` legitimately spawns from the middle of its const block, so
///     a naive "spawns must come last" check would fire on the reference
///     script itself.
///
/// The tokenizer has to understand `;` comments, `"…"` strings and LispBM's
/// `{ … }` brace-progn (used throughout `main.lisp`). Getting that wrong would
/// make the linter reject valid code — and a linter that cries wolf gets
/// switched off, taking the real gate with it.
library;

import 'dart:convert';

import '../ble/vesc/vesc_code_loader.dart';

enum LintLevel { error, warn, info }

class LintIssue {
  final LintLevel level;

  /// 1-based line the issue points at.
  final int line;

  /// Stable machine-readable code (`E_PARENS`, `W_LARGE_QUOTED`, …). This is
  /// what the model sees, so it must not drift.
  final String code;
  final String message;
  final String? hint;

  const LintIssue({
    required this.level,
    required this.line,
    required this.code,
    required this.message,
    this.hint,
  });

  Map<String, dynamic> toJson() => {
        'level': level.name,
        'line': line,
        'code': code,
        'message': message,
        if (hint != null) 'hint': hint,
      };

  @override
  String toString() => '$code@$line: $message';
}

class LintReport {
  final List<LintIssue> issues;

  /// Size the script occupies once packed for upload (what the 120 KiB limit
  /// actually applies to).
  final int packedBytes;

  /// Top-level `(def ...)` count — GET_STATS reports at most 18 of them.
  final int topLevelDefs;
  final int? constStartLine;
  final int? constEndLine;

  const LintReport({
    required this.issues,
    required this.packedBytes,
    required this.topLevelDefs,
    this.constStartLine,
    this.constEndLine,
  });

  List<LintIssue> get errors =>
      [for (final i in issues) if (i.level == LintLevel.error) i];
  List<LintIssue> get warnings =>
      [for (final i in issues) if (i.level == LintLevel.warn) i];
  bool get hasErrors => errors.isNotEmpty;

  Map<String, dynamic> toJson() => {
        'ok': !hasErrors,
        'errors': [for (final i in errors) i.toJson()],
        'warnings': [for (final i in warnings) i.toJson()],
        'info': [
          for (final i in issues)
            if (i.level == LintLevel.info) i.toJson()
        ],
        'packed_bytes': packedBytes,
        'limit_bytes': kLispMaxBytes,
        'top_level_defs': topLevelDefs,
      };
}

/// The VESC's LISP flash area. `uploadCode` asks for `packed + 100` bytes of
/// erase, so that slack counts against the limit too.
const kLispMaxBytes = 120 * 1024;
const _kEraseSlack = 100;

/// Forms that bind a top-level name.
const _definers = {'def', 'defun', 'defunret', 'define'};

LintReport lintLisp(String src) {
  final issues = <LintIssue>[];
  final toks = _tokenize(src, issues);
  final forms = _forms(toks, issues);

  // ---- @const block ----
  int? constStart, constEnd;
  var constStarts = 0, constEnds = 0;
  for (final t in toks) {
    if (t.kind != _TokKind.atom || t.depth != 0) continue;
    if (t.text == '@const-start') {
      constStarts++;
      constStart ??= t.line;
    } else if (t.text == '@const-end') {
      constEnds++;
      constEnd ??= t.line;
    }
  }
  if (constStarts > 1) {
    issues.add(LintIssue(
      level: LintLevel.error,
      line: constStart ?? 1,
      code: 'E_CONST_PAIR',
      message: 'Found $constStarts @const-start markers; there must be at '
          'most one.',
    ));
  }
  if (constStarts == 1 && constEnds != 1) {
    issues.add(LintIssue(
      level: LintLevel.error,
      line: constStart ?? 1,
      code: 'E_CONST_PAIR',
      message: constEnds == 0
          ? '@const-start has no matching @const-end.'
          : 'Found $constEnds @const-end markers for one @const-start.',
    ));
  }
  if (constStarts == 0 && constEnds > 0) {
    issues.add(LintIssue(
      level: LintLevel.error,
      line: constEnd ?? 1,
      code: 'E_CONST_PAIR',
      message: '@const-end without a preceding @const-start.',
    ));
  }
  if (constStart != null && constEnd != null && constEnd < constStart) {
    issues.add(LintIssue(
      level: LintLevel.error,
      line: constEnd,
      code: 'E_CONST_PAIR',
      message: '@const-end appears before @const-start.',
    ));
  }

  // ---- names bound at top level, and where ----
  final boundAt = <String, int>{}; // name -> index of the form that binds it
  final defForms = <_Form>[];
  var topLevelDefs = 0;
  for (var i = 0; i < forms.length; i++) {
    final f = forms[i];
    if (!_definers.contains(f.head)) continue;
    if (f.name != null) boundAt.putIfAbsent(f.name!, () => i);
    defForms.add(f);
    if (f.head == 'def') topLevelDefs++;
  }

  // ---- names that are setq'd anywhere (→ mutable, must stay above @const) ----
  final mutated = <String>{};
  for (var i = 0; i < toks.length - 1; i++) {
    final t = toks[i];
    if (t.kind == _TokKind.atom && (t.text == 'setq' || t.text == 'setvar')) {
      final next = toks[i + 1];
      if (next.kind == _TokKind.atom) mutated.add(next.text);
    }
  }

  for (final f in defForms) {
    final belowConst = constStart != null && f.startLine > constStart;

    if (f.head == 'def' && belowConst && mutated.contains(f.name)) {
      issues.add(LintIssue(
        level: LintLevel.error,
        line: f.startLine,
        code: 'E_MUTABLE_BELOW_CONST',
        message: "'${f.name}' is setq'd at runtime but defined below "
            '@const-start.',
        hint: 'Move the (def ${f.name} …) above @const-start — flashed '
            'bindings are constant.',
      ));
    }

    if (f.head == 'def' && belowConst && f.mentions.contains('bufcreate')) {
      issues.add(LintIssue(
        level: LintLevel.error,
        line: f.startLine,
        code: 'E_BUFCREATE_BELOW_CONST',
        message: "Buffer '${f.name}' is created below @const-start.",
        hint: 'A flashed buffer is read-only — bufset on it fails at runtime. '
            'Move the (def ${f.name} (bufcreate …)) above @const-start.',
      ));
    }

    if (f.head != 'def' && constStart != null && f.startLine < constStart) {
      issues.add(LintIssue(
        level: LintLevel.warn,
        line: f.startLine,
        code: 'W_DEFUN_ABOVE_CONST',
        message: "'${f.name}' is defined above @const-start, so its body "
            'stays in the cons heap.',
        hint: 'Move it inside the @const block; heap exhaustion kills the '
            'script at runtime while the acks still report success.',
      ));
    }

    if (f.head == 'def' && f.quotedLiteralBytes > 512) {
      issues.add(LintIssue(
        level: LintLevel.warn,
        line: f.startLine,
        code: 'W_LARGE_QUOTED',
        message: "'${f.name}' is a ${f.quotedLiteralBytes} B quoted literal.",
        hint: 'The reader builds the whole list on the cons heap BEFORE '
            '@const can flash it, so a big literal can OOM at load time.',
      ));
    }
  }

  // ---- use-before-bind on top-level statements ----
  for (var i = 0; i < forms.length; i++) {
    final f = forms[i];
    if (_definers.contains(f.head)) continue; // definitions may forward-declare
    for (final sym in f.mentions) {
      final at = boundAt[sym];
      if (at != null && at > i) {
        issues.add(LintIssue(
          level: LintLevel.error,
          line: f.startLine,
          code: 'E_USE_BEFORE_BIND',
          message: "Top-level (${f.head} …) references '$sym', which is only "
              'bound on line ${forms[at].startLine}.',
          hint: 'Plain top-level statements run as the file loads. Move this '
              'below the definition — this is why spawns and event-enable '
              'come last.',
        ));
        break; // one report per form is enough to act on
      }
    }
  }

  // ---- event handler / enable pairing ----
  var hasRegister = false, hasEnable = false;
  for (final t in toks) {
    if (t.kind != _TokKind.atom) continue;
    if (t.text == 'event-register-handler') hasRegister = true;
    if (t.text == 'event-enable') hasEnable = true;
  }
  if (hasEnable != hasRegister) {
    issues.add(LintIssue(
      level: LintLevel.warn,
      line: 1,
      code: 'W_EVENT_MISMATCH',
      message: hasEnable
          ? 'event-enable without an event-register-handler — the events have '
              'nowhere to go.'
          : 'event-register-handler without any event-enable — the handler '
              'will never be woken.',
    ));
  }

  // ---- size ----
  final packed = packLispCode(src).length;
  if (packed + _kEraseSlack > kLispMaxBytes) {
    issues.add(LintIssue(
      level: LintLevel.error,
      line: 1,
      code: 'E_TOO_BIG',
      message: 'Script packs to $packed B; the VESC LISP area is '
          '$kLispMaxBytes B (upload also asks for $_kEraseSlack B of erase '
          'slack).',
    ));
  }

  // ---- informational ----
  if (topLevelDefs > 18) {
    issues.add(LintIssue(
      level: LintLevel.info,
      line: 1,
      code: 'I_BINDING_CAP',
      message: 'Script has $topLevelDefs top-level defs; COMM_LISP_GET_STATS '
          'reports at most 18 bindings.',
      hint: 'Debug variables beyond that cap will not be observable from the '
          'phone.',
    ));
  }
  if (constStart == null && utf8.encode(src).length > 4 * 1024) {
    issues.add(const LintIssue(
      level: LintLevel.info,
      line: 1,
      code: 'I_NO_CONST',
      message: 'No @const block in a large script — every definition stays in '
          'the cons heap.',
    ));
  }

  issues.sort((a, b) {
    final byLevel = a.level.index.compareTo(b.level.index);
    return byLevel != 0 ? byLevel : a.line.compareTo(b.line);
  });

  return LintReport(
    issues: issues,
    packedBytes: packed,
    topLevelDefs: topLevelDefs,
    constStartLine: constStart,
    constEndLine: constEnd,
  );
}

// ---------------------------------------------------------------------------
// Tokenizer
// ---------------------------------------------------------------------------

enum _TokKind { open, close, atom, str, quote }

class _Tok {
  final _TokKind kind;
  final String text;
  final int line;
  final int offset;

  /// Nesting depth the token sits at (an `open` carries its outer depth).
  final int depth;

  /// True for `{`/`}` rather than `(`/`)`.
  final bool curly;

  /// This token is inside a quoted expression, so it names nothing.
  final bool quoted;

  const _Tok(this.kind, this.text, this.line, this.offset, this.depth,
      {this.curly = false, this.quoted = false});
}

List<_Tok> _tokenize(String src, List<LintIssue> issues) {
  final toks = <_Tok>[];
  final open = <_Tok>[];

  // Depth at which the innermost pending quote applies; anything deeper than
  // the entry depth is quoted data, not code.
  final quoteDepths = <int>[];
  var pendingQuote = false;

  var line = 1;
  var i = 0;
  var depth = 0;

  bool quotedNow() => quoteDepths.isNotEmpty;

  void closeQuotesAt(int d) {
    while (quoteDepths.isNotEmpty && quoteDepths.last >= d) {
      quoteDepths.removeLast();
    }
  }

  while (i < src.length) {
    final c = src[i];

    if (c == '\n') {
      line++;
      i++;
      continue;
    }
    if (c.trim().isEmpty) {
      i++;
      continue;
    }
    if (c == ';') {
      while (i < src.length && src[i] != '\n') {
        i++;
      }
      continue;
    }
    if (c == '"') {
      final start = i;
      final startLine = line;
      i++;
      while (i < src.length) {
        if (src[i] == '\\') {
          if (src[i + 1 < src.length ? i + 1 : i] == '\n') line++;
          i += 2;
          continue;
        }
        if (src[i] == '"') {
          i++;
          break;
        }
        if (src[i] == '\n') line++;
        i++;
      }
      toks.add(_Tok(_TokKind.str, src.substring(start, i), startLine, start,
          depth, quoted: quotedNow()));
      if (pendingQuote) {
        pendingQuote = false;
      }
      continue;
    }
    if (c == "'") {
      toks.add(_Tok(_TokKind.quote, c, line, i, depth, quoted: quotedNow()));
      pendingQuote = true;
      i++;
      continue;
    }
    if (c == '(' || c == '{') {
      final tok = _Tok(_TokKind.open, c, line, i, depth,
          curly: c == '{', quoted: quotedNow());
      toks.add(tok);
      open.add(tok);
      if (pendingQuote) {
        // The quote covers this whole list.
        quoteDepths.add(depth);
        pendingQuote = false;
      }
      depth++;
      i++;
      continue;
    }
    if (c == ')' || c == '}') {
      depth--;
      if (depth < 0) {
        issues.add(LintIssue(
          level: LintLevel.error,
          line: line,
          code: 'E_PARENS',
          message: "Unbalanced '$c' — closing more than was opened.",
        ));
        depth = 0;
        i++;
        continue;
      }
      final opener = open.isNotEmpty ? open.removeLast() : null;
      if (opener != null && opener.curly != (c == '}')) {
        issues.add(LintIssue(
          level: LintLevel.error,
          line: line,
          code: 'E_PARENS',
          message: "'$c' closes a '${opener.curly ? '{' : '('}' opened on "
              'line ${opener.line}.',
        ));
      }
      closeQuotesAt(depth);
      toks.add(_Tok(_TokKind.close, c, line, i, depth,
          curly: c == '}', quoted: quotedNow()));
      i++;
      continue;
    }

    // Atom: run up to the next delimiter.
    final start = i;
    while (i < src.length) {
      final d = src[i];
      if (d.trim().isEmpty ||
          d == '(' ||
          d == ')' ||
          d == '{' ||
          d == '}' ||
          d == ';' ||
          d == '"' ||
          d == "'") {
        break;
      }
      i++;
    }
    final wasQuoted = quotedNow() || pendingQuote;
    pendingQuote = false;
    toks.add(_Tok(_TokKind.atom, src.substring(start, i), line, start, depth,
        quoted: wasQuoted));
  }

  if (depth != 0 && open.isNotEmpty) {
    issues.add(LintIssue(
      level: LintLevel.error,
      line: open.first.line,
      code: 'E_PARENS',
      message: "'${open.first.curly ? '{' : '('}' opened on line "
          '${open.first.line} is never closed (depth $depth at end of file).',
    ));
  }

  return toks;
}

// ---------------------------------------------------------------------------
// Top-level forms
// ---------------------------------------------------------------------------

class _Form {
  final int startLine;
  final int endLine;

  /// First atom of the form: `def`, `defun`, `spawn`, `event-enable`, …
  final String head;

  /// For a definer, the name it binds.
  final String? name;

  /// Unquoted symbols the form references (used for use-before-bind).
  final Set<String> mentions;

  /// Size of the largest quoted list literal inside the form.
  final int quotedLiteralBytes;

  const _Form({
    required this.startLine,
    required this.endLine,
    required this.head,
    required this.name,
    required this.mentions,
    required this.quotedLiteralBytes,
  });
}

List<_Form> _forms(List<_Tok> toks, List<LintIssue> issues) {
  final forms = <_Form>[];
  var i = 0;
  while (i < toks.length) {
    final t = toks[i];
    if (t.kind != _TokKind.open || t.depth != 0) {
      i++;
      continue;
    }
    // Collect through the matching close.
    final body = <_Tok>[];
    var depth = 0;
    var j = i;
    for (; j < toks.length; j++) {
      final u = toks[j];
      if (u.kind == _TokKind.open) depth++;
      if (u.kind == _TokKind.close) {
        depth--;
        if (depth == 0) break;
      }
      body.add(u);
    }
    final head = body.length > 1 && body[1].kind == _TokKind.atom
        ? body[1].text
        : '';
    String? name;
    if (_definers.contains(head) && body.length > 2) {
      // (def NAME …) / (defun NAME (args) …)
      final n = body[2];
      if (n.kind == _TokKind.atom) {
        name = n.text;
      } else if (n.kind == _TokKind.open && body.length > 3) {
        name = body[3].kind == _TokKind.atom ? body[3].text : null;
      }
    }

    final mentions = <String>{};
    for (var k = 2; k < body.length; k++) {
      final u = body[k];
      if (u.kind == _TokKind.atom && !u.quoted && !_isLiteral(u.text)) {
        mentions.add(u.text);
      }
    }

    var quotedBytes = 0;
    for (var k = 0; k < body.length; k++) {
      if (body[k].kind != _TokKind.quote) continue;
      if (k + 1 >= body.length || body[k + 1].kind != _TokKind.open) continue;
      final end = _matchingClose(toks, i + k + 1);
      if (end != null) {
        final span = toks[end].offset - body[k].offset;
        if (span > quotedBytes) quotedBytes = span;
      }
    }

    forms.add(_Form(
      startLine: t.line,
      endLine: j < toks.length ? toks[j].line : t.line,
      head: head,
      name: name,
      mentions: mentions,
      quotedLiteralBytes: quotedBytes,
    ));
    i = j + 1;
  }
  return forms;
}

int? _matchingClose(List<_Tok> toks, int openIdx) {
  var depth = 0;
  for (var i = openIdx; i < toks.length; i++) {
    if (toks[i].kind == _TokKind.open) depth++;
    if (toks[i].kind == _TokKind.close) {
      depth--;
      if (depth == 0) return i;
    }
  }
  return null;
}

/// Numbers, keywords and syntax that can't name a top-level binding.
bool _isLiteral(String s) {
  if (s.isEmpty) return true;
  if (s == 'nil' || s == 't' || s == '_' || s == '.' || s == '?') return true;
  if (s.startsWith('@')) return true;
  return double.tryParse(s) != null ||
      int.tryParse(s.replaceFirst('0x', ''), radix: 16) != null &&
          s.startsWith('0x');
}
