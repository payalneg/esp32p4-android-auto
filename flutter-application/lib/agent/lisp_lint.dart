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
///   * The quick-action panel frames must match what `vesc_lisp_panel.c`
///     decodes byte for byte. An unknown control type, a count byte that
///     disagrees with the rows listed, an id that only half the three panel
///     functions know about, an over-long label or a frame wider than its
///     buffer are all dropped or truncated ON THE P4, which reports nothing
///     back over BLE — the row just isn't there.
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

  // ---- quick-action panel frames ----
  // Skipped outright when the parens are already broken: the parent/sibling
  // bookkeeping these checks rely on is meaningless on an unbalanced file.
  if (!issues.any((i) => i.code == 'E_PARENS')) {
    _panelChecks(toks, forms, issues);
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

  /// Index of this form's opening token, and of its matching close. Rules that
  /// need more than a flat `mentions` set walk `toks[tokStart .. tokEnd)` —
  /// absolute indices, so `_matchingClose` works on them directly.
  final int tokStart;
  final int tokEnd;

  const _Form({
    required this.startLine,
    required this.endLine,
    required this.head,
    required this.name,
    required this.mentions,
    required this.quotedLiteralBytes,
    required this.tokStart,
    required this.tokEnd,
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
      tokStart: i,
      tokEnd: j,
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

// ---------------------------------------------------------------------------
// Quick-action panel frames
// ---------------------------------------------------------------------------
//
// The drawer on the head unit is drawn from bytes this script emits; the P4
// decodes them with no schema and no way to ask again (see
// components/vesc_can/vesc_lisp_panel.c). Every deviation below is dropped or
// truncated on its side WITHOUT any error reaching the phone, so the model
// gets a clean flash, a clean verify, and a control that silently isn't there.
// That is exactly the class of bug this linter exists to catch.

/// Buffer-append helpers a panel reply is built from.
const _emitOps = {'pu8', 'pi32', 'pstr'};

/// Control types the P4 knows (vlp_ctrl_type_t). Anything else makes the tail
/// size unknowable, so it stops decoding and drops the rest of the frame.
const _ctrlToggle = 1, _ctrlButton = 2, _ctrlNumber = 3, _ctrlLabel = 4;

/// VLP_LABEL_MAX / VLP_SUFFIX_MAX minus the NUL the P4 reserves.
const _labelMaxBytes = 39, _suffixMaxBytes = 11;

/// VLP_MAX_CTRLS.
const _maxCtrls = 16;

/// One `(pu8 …)` / `(pi32 …)` / `(pstr …)` call inside a form.
class _Emit {
  final String op;
  final int openIdx;
  final int closeIdx;

  /// Token index of the enclosing open — the brace-progn in practice. Two
  /// emits are siblings only if this matches; `_Tok.depth` is not enough,
  /// because an `open` carries its OUTER depth and both arms of an `(if …)`
  /// therefore sit at the same depth.
  final int parent;

  const _Emit(this.op, this.openIdx, this.closeIdx, this.parent);
}

/// A control as declared in UI_DESC. `id` and `label` are null when written as
/// an expression rather than a literal.
class _Ctrl {
  final int? id;
  final int type;
  final String? label;
  final int line;
  const _Ctrl(this.id, this.type, this.label, this.line);
}

class _UiRun {
  final List<_Ctrl> controls;
  final bool malformed;
  final int? declaredCount;
  const _UiRun(this.controls, this.malformed, this.declaredCount);
}

class _StateRun {
  final List<({int? id, int line})> ids;
  final bool malformed;
  final int? declaredCount;
  const _StateRun(this.ids, this.malformed, this.declaredCount);
}

int? _intAtom(String s) {
  if (s.startsWith('0x') || s.startsWith('0X')) {
    return int.tryParse(s.substring(2), radix: 16);
  }
  return int.tryParse(s);
}

/// The emit's argument as a literal int, or null when it is an expression
/// (`(pu8 (if (= throttle-on 1) 1 0))`). Null disables the rules that need the
/// value; it never invents one.
int? _emitInt(List<_Tok> toks, _Emit e) {
  if (e.closeIdx != e.openIdx + 3) return null;
  final a = toks[e.openIdx + 2];
  return a.kind == _TokKind.atom ? _intAtom(a.text) : null;
}

/// The emit's argument as a literal string, quotes stripped.
String? _emitStr(List<_Tok> toks, _Emit e) {
  if (e.closeIdx != e.openIdx + 3) return null;
  final a = toks[e.openIdx + 2];
  if (a.kind != _TokKind.str || a.text.length < 2) return null;
  return a.text.substring(1, a.text.length - 1);
}

/// Bytes the string occupies on the wire, NOT counting its NUL terminator.
int _strBytes(String content) {
  final sb = StringBuffer();
  for (var i = 0; i < content.length; i++) {
    if (content[i] == r'\' && i + 1 < content.length) i++;
    sb.write(content[i]);
  }
  return utf8.encode(sb.toString()).length;
}

/// Emits of a form, in source order, plus every foreign direct child (used by
/// the interleave gate). Returns null if the form's parens don't resolve.
({List<_Emit> emits, List<({int parent, int openIdx})> others})? _scanEmits(
    List<_Tok> toks, _Form f) {
  final emits = <_Emit>[];
  final others = <({int parent, int openIdx})>[];
  final stack = <int>[];
  var k = f.tokStart;
  while (k < f.tokEnd) {
    final t = toks[k];
    if (t.kind == _TokKind.open) {
      final isEmit = k + 1 < f.tokEnd &&
          toks[k + 1].kind == _TokKind.atom &&
          !toks[k + 1].quoted &&
          _emitOps.contains(toks[k + 1].text);
      if (isEmit) {
        final close = _matchingClose(toks, k);
        if (close == null || close >= f.tokEnd) return null;
        emits.add(_Emit(toks[k + 1].text, k, close,
            stack.isEmpty ? f.tokStart : stack.last));
        // Skip the whole call: opens inside the ARGUMENT must not register as
        // parents, or `(pu8 (if …))` would look like a nested emit context.
        k = close + 1;
        continue;
      }
      if (stack.isNotEmpty) others.add((parent: stack.last, openIdx: k));
      stack.add(k);
      k++;
      continue;
    }
    if (t.kind == _TokKind.close) {
      if (stack.isNotEmpty) stack.removeLast();
      k++;
      continue;
    }
    k++;
  }
  return (emits: emits, others: others);
}

/// Controls declared by one UI_DESC run. Reports `E_PANEL_CTRL_TYPE` itself,
/// because an unknown type is also what forces the walk to stop.
_UiRun _parseUiRun(List<_Tok> toks, List<_Emit> body, List<_Emit> suffixes,
    List<LintIssue> issues) {
  if (body.length < 2 || body[0].op != 'pu8' || body[1].op != 'pu8') {
    return const _UiRun([], true, null);
  }
  final declaredCount = _emitInt(toks, body[1]);
  final controls = <_Ctrl>[];
  var malformed = false;
  var i = 2;
  while (i < body.length) {
    if (i + 2 >= body.length ||
        body[i].op != 'pu8' ||
        body[i + 1].op != 'pu8' ||
        body[i + 2].op != 'pstr') {
      malformed = true;
      break;
    }
    final id = _emitInt(toks, body[i]);
    final type = _emitInt(toks, body[i + 1]);
    final label = _emitStr(toks, body[i + 2]);
    final line = toks[body[i].openIdx].line;
    if (type == null) {
      // An expression in the type slot: we cannot know the tail length, so
      // stop rather than guess. No diagnostic — this shape is legal.
      malformed = true;
      break;
    }
    i += 3;
    if (type == _ctrlToggle) {
      if (i < body.length && body[i].op == 'pu8') {
        i += 1;
      } else {
        malformed = true;
      }
    } else if (type == _ctrlButton) {
      // No tail.
    } else if (type == _ctrlNumber) {
      if (i + 4 < body.length &&
          body[i].op == 'pi32' &&
          body[i + 1].op == 'pi32' &&
          body[i + 2].op == 'pi32' &&
          body[i + 3].op == 'pi32' &&
          body[i + 4].op == 'pstr') {
        suffixes.add(body[i + 4]);
        i += 5;
      } else {
        malformed = true;
      }
    } else if (type == _ctrlLabel) {
      if (i + 1 < body.length &&
          body[i].op == 'pi32' &&
          body[i + 1].op == 'pstr') {
        suffixes.add(body[i + 1]);
        i += 2;
      } else {
        malformed = true;
      }
    } else {
      final swapped = id != null &&
          id >= _ctrlToggle &&
          id <= _ctrlLabel &&
          (type < _ctrlToggle || type > _ctrlLabel);
      issues.add(LintIssue(
        level: LintLevel.error,
        line: line,
        code: 'E_PANEL_CTRL_TYPE',
        message: 'Panel control declares type $type; the P4 only knows '
            '1=toggle, 2=button, 3=number, 4=label.',
        hint: '${swapped ? 'The order is (pu8 <id>) (pu8 <type>) '
            '(pstr "label") — these two look swapped: $id is a valid type '
            'and $type is not. ' : ''}An unknown type has no known tail '
            'size, so the P4 stops decoding there and SILENTLY drops this '
            'control and every control after it — no error, the rows just '
            'never appear.',
      ));
      malformed = true;
    }
    if (type >= _ctrlToggle && type <= _ctrlLabel) {
      controls.add(_Ctrl(id, type, label, line));
    }
    if (malformed) break;
  }
  return _UiRun(controls, malformed, declaredCount);
}

/// `(pu8 id) (pi32 value)` pairs of one STATE run.
_StateRun _parseStateRun(List<_Tok> toks, List<_Emit> body) {
  if (body.isEmpty || body[0].op != 'pu8') {
    return const _StateRun([], true, null);
  }
  final declaredCount = _emitInt(toks, body[0]);
  final ids = <({int? id, int line})>[];
  var malformed = false;
  var i = 1;
  while (i < body.length) {
    if (i + 1 >= body.length ||
        body[i].op != 'pu8' ||
        body[i + 1].op != 'pi32') {
      malformed = true;
      break;
    }
    ids.add((id: _emitInt(toks, body[i]), line: toks[body[i].openIdx].line));
    i += 2;
  }
  return _StateRun(ids, malformed, declaredCount);
}

/// Formal parameter names of `(defun NAME (a b) …)`.
Set<String> _formals(List<_Tok> toks, _Form f) {
  final out = <String>{};
  final ai = f.tokStart + 3;
  if (ai >= f.tokEnd || toks[ai].kind != _TokKind.open) return out;
  final close = _matchingClose(toks, ai);
  if (close == null) return out;
  for (var k = ai + 1; k < close; k++) {
    if (toks[k].kind == _TokKind.atom) out.add(toks[k].text);
  }
  return out;
}

/// Literals compared against `param` with `=` inside the form: the control ids
/// a `cond` dispatches on.
List<({int id, int line})> _dispatchIds(
    List<_Tok> toks, _Form f, String param) {
  final out = <({int id, int line})>[];
  for (var k = f.tokStart; k + 4 < f.tokEnd; k++) {
    if (toks[k].kind != _TokKind.open) continue;
    if (toks[k + 1].kind != _TokKind.atom || toks[k + 1].text != '=') continue;
    if (toks[k + 4].kind != _TokKind.close) continue;
    final a = toks[k + 2], b = toks[k + 3];
    if (a.kind != _TokKind.atom || b.kind != _TokKind.atom) continue;
    if (a.text == param) {
      final v = _intAtom(b.text);
      if (v != null) out.add((id: v, line: a.line));
    } else if (b.text == param) {
      final v = _intAtom(a.text);
      if (v != null) out.add((id: v, line: b.line));
    }
  }
  return out;
}

/// Name of the single buffer this form sends, or null if it sends none or
/// several.
String? _sendDataBuf(List<_Tok> toks, _Form f) {
  final names = <String>{};
  for (var k = f.tokStart; k + 2 < f.tokEnd; k++) {
    if (toks[k].kind == _TokKind.open &&
        toks[k + 1].kind == _TokKind.atom &&
        toks[k + 1].text == 'send-data' &&
        toks[k + 2].kind == _TokKind.atom) {
      names.add(toks[k + 2].text);
    }
  }
  return names.length == 1 ? names.first : null;
}

/// `(def NAME (bufcreate N))` → N.
int? _bufCapacity(List<_Tok> toks, List<_Form> forms, String name) {
  for (final f in forms) {
    if (f.head != 'def' || f.name != name) continue;
    for (var k = f.tokStart; k + 2 < f.tokEnd; k++) {
      if (toks[k].kind == _TokKind.open &&
          toks[k + 1].kind == _TokKind.atom &&
          toks[k + 1].text == 'bufcreate' &&
          toks[k + 2].kind == _TokKind.atom) {
        return _intAtom(toks[k + 2].text);
      }
    }
  }
  return null;
}

/// Panel-protocol checks. Every one of them degrades to SILENCE when the shape
/// is not recognised: this linter gates flashing, and a false positive here
/// costs more than a missed catch — a gate that cries wolf gets switched off.
void _panelChecks(
    List<_Tok> toks, List<_Form> forms, List<LintIssue> issues) {
  final declaredIds = <int>{};
  final declared = <_Ctrl>[];
  final stateIds = <int, int>{}; // control id -> line it is sent from
  final actionIds = <({int id, int line})>[];
  var sawUi = false;
  var declaredComplete = true;
  var stateUsable = true;

  for (final f in forms) {
    final scan = _scanEmits(toks, f);
    if (scan == null) continue;
    final emits = scan.emits;
    if (emits.length < 3) continue;

    // Split the form's emits into frames at each 'V' 'P' <msg> triple.
    final starts = <int>[];
    for (var e = 0; e + 2 < emits.length; e++) {
      if (emits[e].op != 'pu8' ||
          emits[e + 1].op != 'pu8' ||
          emits[e + 2].op != 'pu8') {
        continue;
      }
      if (_emitInt(toks, emits[e]) == 0x56 &&
          _emitInt(toks, emits[e + 1]) == 0x50) {
        starts.add(e);
      }
    }

    for (var s = 0; s < starts.length; s++) {
      final from = starts[s];
      final to = s + 1 < starts.length ? starts[s + 1] : emits.length;
      final run = emits.sublist(from, to);

      // G1: one parent for the whole run. Kills rows emitted from inside an
      // (if …) or a loop, where counting them statically is meaningless.
      final parent = run.first.parent;
      if (run.any((e) => e.parent != parent)) continue;

      // G2: no foreign direct sibling between the first and last emit. This is
      // what stops a helper call — (emit-row 2 "B") — from being miscounted.
      final lo = run.first.openIdx, hi = run.last.openIdx;
      final interleaved = scan.others.any(
          (o) => o.parent == parent && o.openIdx > lo && o.openIdx < hi);
      if (interleaved) continue;

      final msg = _emitInt(toks, run[2]);
      final body = run.sublist(3);
      final suffixes = <_Emit>[];

      if (msg == 0x81) {
        sawUi = true;
        final ui = _parseUiRun(toks, body, suffixes, issues);
        if (ui.malformed || ui.controls.any((c) => c.id == null)) {
          declaredComplete = false;
        }
        for (final c in ui.controls) {
          if (c.id == null) continue;
          if (!declaredIds.add(c.id!)) {
            issues.add(LintIssue(
              level: LintLevel.error,
              line: c.line,
              code: 'E_PANEL_DUP_ID',
              message: 'Panel control id ${c.id} is declared twice.',
              hint: 'The id is the P4\'s only handle on a row: both STATE '
                  'updates and taps resolve to the FIRST match, so the second '
                  'row never updates and the first one gets its taps.',
            ));
          }
          declared.add(c);
        }
        if (!ui.malformed &&
            ui.declaredCount != null &&
            ui.declaredCount != ui.controls.length) {
          issues.add(LintIssue(
            level: LintLevel.error,
            line: toks[run.first.openIdx].line,
            code: 'E_PANEL_COUNT',
            message: 'UI_DESC count byte says ${ui.declaredCount} controls '
                'but ${ui.controls.length} are listed.',
            hint: 'The byte after (pu8 0x81) (pu8 <ver>) must equal the number '
                'of controls that follow. Too low and the extra rows never '
                'appear; too high and the P4 decodes leftovers from the '
                'previous, longer reply — the buffer is never cleared, only '
                'pi is reset. Both fail silently.',
          ));
        }
        final n = ui.declaredCount ?? ui.controls.length;
        if (n > _maxCtrls) {
          issues.add(LintIssue(
            level: LintLevel.warn,
            line: toks[run.first.openIdx].line,
            code: 'W_PANEL_TOO_MANY',
            message: 'UI_DESC declares $n controls; the P4 renders at most '
                '$_maxCtrls.',
            hint: 'Controls past the ${_maxCtrls}th are dropped without any '
                'error.',
          ));
        }
        for (final c in ui.controls) {
          if (c.label == null) continue;
          final b = _strBytes(c.label!);
          if (b > _labelMaxBytes) {
            issues.add(LintIssue(
              level: LintLevel.error,
              line: c.line,
              code: 'E_PANEL_STR_LEN',
              message: 'Panel label "${c.label}" is $b bytes; the P4 field '
                  'holds $_labelMaxBytes plus the NUL.',
              hint: 'An over-long string leaves the P4 decoder stopped mid-'
                  'string instead of past the terminator, so every byte after '
                  'it is misread and the rest of the panel is garbage.',
            ));
          }
        }
        for (final sfx in suffixes) {
          final s = _emitStr(toks, sfx);
          if (s == null) continue;
          final b = _strBytes(s);
          if (b > _suffixMaxBytes) {
            issues.add(LintIssue(
              level: LintLevel.error,
              line: toks[sfx.openIdx].line,
              code: 'E_PANEL_STR_LEN',
              message: 'Panel unit suffix "$s" is $b bytes; the P4 field holds '
                  '$_suffixMaxBytes plus the NUL.',
              hint: 'An over-long string leaves the P4 decoder stopped mid-'
                  'string instead of past the terminator, so every byte after '
                  'it is misread and the rest of the panel is garbage.',
            ));
          }
        }
      } else if (msg == 0x82) {
        final st = _parseStateRun(toks, body);
        if (st.malformed || st.ids.any((v) => v.id == null)) {
          stateUsable = false;
        }
        for (final e in st.ids) {
          if (e.id != null) stateIds[e.id!] = e.line;
        }
        if (!st.malformed &&
            st.declaredCount != null &&
            st.declaredCount != st.ids.length) {
          issues.add(LintIssue(
            level: LintLevel.error,
            line: toks[run.first.openIdx].line,
            code: 'E_PANEL_COUNT',
            message: 'STATE count byte says ${st.declaredCount} entries but '
                '${st.ids.length} are listed.',
            hint: 'The byte after (pu8 0x82) must equal the number of '
                '(pu8 id) (pi32 value) pairs that follow.',
          ));
        }
      }

      // Frame size vs the buffer it is written into. Skipped unless every
      // string in the run is a literal — otherwise the sum is a guess.
      final bufName = _sendDataBuf(toks, f);
      if (bufName == null) continue;
      final cap = _bufCapacity(toks, forms, bufName);
      if (cap == null) continue;
      var bytes = 0;
      var sizeKnown = true;
      for (final e in run) {
        if (e.op == 'pu8') {
          bytes += 1;
        } else if (e.op == 'pi32') {
          bytes += 4;
        } else {
          final s = _emitStr(toks, e);
          if (s == null) {
            sizeKnown = false;
            break;
          }
          bytes += _strBytes(s) + 1; // buflen counts the NUL
        }
      }
      if (sizeKnown && bytes > cap) {
        issues.add(LintIssue(
          level: LintLevel.error,
          line: toks[run.first.openIdx].line,
          code: 'E_PANEL_BUF_OVERFLOW',
          message: 'This frame writes $bytes B into \'$bufName\', which is '
              '(bufcreate $cap).',
          hint: 'The out-of-range bufset is an evaluation error that kills the '
              'event thread, so the panel simply stops answering and the '
              'drawer stays empty. Grow the (bufcreate $cap) — it lives above '
              '@const-start — but only as far as needed: the whole buffer goes '
              'on the wire on every reply.',
        ));
      }
    }
  }

  // Action ids, name-anchored on the `cid` convention. Corroborated PER FORM:
  // a dispatcher counts as the panel's only if at least one of its literals is
  // a declared control id. `cid` also reads as "CAN id" in this project, and
  // (defun on-frame (cid data) … (= cid 42) …) must not look like a panel bug
  // just because some other function happens to be the real panel-action.
  var actionsUsable = false;
  for (final f in forms) {
    if (f.head != 'defun' && f.head != 'defunret') continue;
    final formals = _formals(toks, f);
    final param = formals.contains('cid')
        ? 'cid'
        : formals.contains('ctrl-id')
            ? 'ctrl-id'
            : null;
    if (param == null) continue;
    final ids = _dispatchIds(toks, f, param);
    if (!ids.any((a) => declaredIds.contains(a.id))) continue;
    actionsUsable = true;
    actionIds.addAll(ids);
  }

  if (!sawUi || !declaredComplete) return;

  for (final entry in stateIds.entries) {
    final id = entry.key;
    if (declaredIds.contains(id)) continue;
    issues.add(LintIssue(
      level: LintLevel.error,
      line: entry.value,
      code: 'E_PANEL_STATE_ID',
      message: 'STATE sends a value for control id $id, which no UI_DESC '
          'declares.',
      hint: 'The P4 looks the id up in the descriptor and drops the value when '
          'it is missing, so that row silently never updates. Declare it in '
          'panel-send-ui (id, type, label) and bump that count byte, or drop '
          'the pair.',
    ));
  }

  if (actionsUsable) {
    for (final a in actionIds) {
      if (declaredIds.contains(a.id)) continue;
      issues.add(LintIssue(
        level: LintLevel.error,
        line: a.line,
        code: 'E_PANEL_ACTION_ID',
        message: 'Action branch (= cid ${a.id}) handles a control id that no '
            'UI_DESC declares.',
        hint: 'The P4 only ever sends ids it was told about, so this branch is '
            'unreachable and whatever control it was meant for does nothing '
            'when tapped. Add it to panel-send-ui and bump the count byte.',
      ));
    }
  }

  if (stateUsable && actionsUsable) {
    final acted = {for (final a in actionIds) a.id};
    for (final c in declared) {
      if (c.type == _ctrlLabel) continue; // a static readout needs neither
      if (c.id == null) continue;
      if (stateIds.containsKey(c.id) || acted.contains(c.id)) continue;
      issues.add(LintIssue(
        level: LintLevel.info,
        line: c.line,
        code: 'I_PANEL_CTRL_INERT',
        message: 'Panel control id ${c.id}'
            '${c.label != null ? ' ("${c.label}")' : ''} has no STATE entry '
            'and no (= cid ${c.id}) action branch.',
        hint: 'It renders but never updates and does nothing when tapped. A '
            'panel change is three edits: panel-send-ui, panel-send-state and '
            'panel-action.',
      ));
    }
  }
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
