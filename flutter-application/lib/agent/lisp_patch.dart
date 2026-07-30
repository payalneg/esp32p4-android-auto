/// SEARCH/REPLACE patching for the LISP working copy.
///
/// The reference script is ~550 lines / 23 KB. Whole-file rewrites cost ~8K
/// output tokens per edit and risk the model silently dropping a `defun`;
/// line-numbered hunks drift the moment the first one lands. Anchored
/// search/replace blocks avoid both.
///
/// Wire format (one or more blocks, in the `patch` string argument):
///
///     <<<<<<< SEARCH
///     (def cruise-kp 0.02)
///     =======
///     (def cruise-kp 0.035)
///     >>>>>>> REPLACE
///
/// Contract, enforced here and stated in the system prompt:
///   * SEARCH is byte-exact, including indentation, and must match EXACTLY
///     once — include surrounding lines until it is unique.
///   * An empty SEARCH is rejected. Blind appending would land text after
///     `@const-end`, which is the last line of the reference script — a silent
///     killer.
///   * Blocks apply transactionally: all or nothing.
///
/// There is deliberately no fuzzy content matching. The fallback pass only
/// forgives CRLF and trailing whitespace; anything looser silently edits the
/// wrong `defun`.
library;

import 'dart:convert';
import 'dart:math' as math;

class PatchBlock {
  final String search;
  final String replace;
  final int index;
  const PatchBlock(
      {required this.search, required this.replace, required this.index});
}

enum PatchErrorKind { malformed, emptySearch, noMatch, ambiguous, tooLarge }

/// A near-miss for a SEARCH that didn't match, so the model can see what the
/// file actually says instead of guessing again.
class PatchCandidate {
  final int line;
  final String snippet;
  final double similarity;
  const PatchCandidate(this.line, this.snippet, this.similarity);

  Map<String, dynamic> toJson() => {
        'line': line,
        'snippet': snippet,
        'similarity': double.parse(similarity.toStringAsFixed(2)),
      };
}

class PatchError {
  final int block;
  final PatchErrorKind kind;
  final String message;
  final List<PatchCandidate> candidates;

  /// For [PatchErrorKind.ambiguous]: the lines SEARCH matched at.
  final List<int> matchLines;

  const PatchError({
    required this.block,
    required this.kind,
    required this.message,
    this.candidates = const [],
    this.matchLines = const [],
  });

  Map<String, dynamic> toJson() => {
        'block': block,
        'kind': kind.name,
        'message': message,
        if (candidates.isNotEmpty)
          'candidates': [for (final c in candidates) c.toJson()],
        if (matchLines.isNotEmpty) 'match_lines': matchLines,
      };
}

class PatchResult {
  final bool ok;

  /// The patched text — null when [ok] is false (nothing is half-applied).
  final String? text;
  final int applied;
  final List<PatchError> errors;
  final String diff;

  /// A block only matched after CRLF / trailing-whitespace normalisation.
  /// Reported back so the model learns its whitespace was off.
  final bool usedNormalisedMatch;

  const PatchResult({
    required this.ok,
    required this.text,
    required this.applied,
    required this.errors,
    required this.diff,
    required this.usedNormalisedMatch,
  });

  static const _hint =
      'SEARCH must match the file byte-exactly and exactly once. Call '
      'read_lines around the target and copy the text verbatim, WITHOUT the '
      'line-number gutter.';

  Map<String, dynamic> toJson() => {
        'ok': ok,
        'applied': applied,
        if (ok) 'diff': diff,
        if (usedNormalisedMatch) 'used_normalised_match': true,
        if (!ok) 'errors': [for (final e in errors) e.toJson()],
        if (!ok) 'hint': _hint,
      };
}

// Anything that isn't one of these three markers is ignored, so markdown
// fences and prose around the blocks cost nothing.
final _searchRe = RegExp(r'^\s*<{5,}\s*SEARCH\s*$', caseSensitive: false);
final _sepRe = RegExp(r'^\s*={5,}\s*$');
final _replaceRe = RegExp(r'^\s*>{5,}\s*REPLACE\s*$', caseSensitive: false);

/// Pull the blocks out of whatever the model produced. Tolerates markdown
/// fences, CRLF, and prose before/between/after the blocks.
List<PatchBlock> parsePatch(String raw) {
  final blocks = <PatchBlock>[];
  final lines = raw.replaceAll('\r\n', '\n').split('\n');

  var i = 0;
  while (i < lines.length) {
    if (!_searchRe.hasMatch(lines[i])) {
      i++;
      continue;
    }
    i++;
    final search = <String>[];
    var sawSep = false;
    while (i < lines.length) {
      if (_sepRe.hasMatch(lines[i])) {
        sawSep = true;
        i++;
        break;
      }
      search.add(lines[i]);
      i++;
    }
    if (!sawSep) break; // unterminated — reported by applyPatch
    final replace = <String>[];
    var sawEnd = false;
    while (i < lines.length) {
      if (_replaceRe.hasMatch(lines[i])) {
        sawEnd = true;
        i++;
        break;
      }
      replace.add(lines[i]);
      i++;
    }
    if (!sawEnd) break;
    blocks.add(PatchBlock(
      search: search.join('\n'),
      replace: replace.join('\n'),
      index: blocks.length,
    ));
  }
  return blocks;
}

/// Apply [rawPatch] to [source]. Never returns partially-applied text.
PatchResult applyPatch(String source, String rawPatch,
    {int maxBytes = 120 * 1024}) {
  final blocks = parsePatch(rawPatch);
  final errors = <PatchError>[];

  if (blocks.isEmpty) {
    final stray = rawPatch.split('\n').any((l) =>
        _searchRe.hasMatch(l) || _sepRe.hasMatch(l) || _replaceRe.hasMatch(l));
    errors.add(PatchError(
      block: 0,
      kind: PatchErrorKind.malformed,
      message: stray
          ? 'A block was opened but never closed. Each edit needs all three '
              'markers: <<<<<<< SEARCH, =======, >>>>>>> REPLACE.'
          : 'No SEARCH/REPLACE block found in the patch.',
    ));
    return PatchResult(
        ok: false,
        text: null,
        applied: 0,
        errors: errors,
        diff: '',
        usedNormalisedMatch: false);
  }

  var scratch = source;
  var applied = 0;
  var normalised = false;

  for (final b in blocks) {
    if (b.search.trim().isEmpty) {
      errors.add(PatchError(
        block: b.index,
        kind: PatchErrorKind.emptySearch,
        message: 'Empty SEARCH. To insert, anchor on a real line and re-emit '
            'it in REPLACE — appending blind would land text after '
            '@const-end.',
      ));
      continue;
    }

    final hits = _findAll(scratch, b.search);
    if (hits.length == 1) {
      scratch = scratch.replaceRange(
          hits.first, hits.first + b.search.length, b.replace);
      applied++;
      continue;
    }
    if (hits.length > 1) {
      errors.add(PatchError(
        block: b.index,
        kind: PatchErrorKind.ambiguous,
        message: 'SEARCH matches ${hits.length} places. Include more '
            'surrounding lines so it is unique.',
        matchLines: [for (final h in hits) _lineOf(scratch, h)],
      ));
      continue;
    }

    // Second pass: forgive CRLF and trailing whitespace only.
    final soft = _findNormalised(scratch, b.search);
    if (soft.length == 1) {
      final (start, end) = soft.first;
      scratch = scratch.replaceRange(start, end, b.replace);
      applied++;
      normalised = true;
      continue;
    }
    if (soft.length > 1) {
      errors.add(PatchError(
        block: b.index,
        kind: PatchErrorKind.ambiguous,
        message: 'SEARCH matches ${soft.length} places once whitespace is '
            'normalised. Include more surrounding lines.',
        matchLines: [for (final s in soft) _lineOf(scratch, s.$1)],
      ));
      continue;
    }

    errors.add(PatchError(
      block: b.index,
      kind: PatchErrorKind.noMatch,
      message: 'SEARCH not found in the file.',
      candidates: _candidates(scratch, b.search),
    ));
  }

  if (errors.isNotEmpty) {
    return PatchResult(
        ok: false,
        text: null,
        applied: 0,
        errors: errors,
        diff: '',
        usedNormalisedMatch: normalised);
  }

  final bytes = utf8.encode(scratch).length;
  if (bytes > maxBytes) {
    return PatchResult(
      ok: false,
      text: null,
      applied: 0,
      errors: [
        PatchError(
          block: 0,
          kind: PatchErrorKind.tooLarge,
          message: 'Result is $bytes B, over the $maxBytes B limit.',
        )
      ],
      diff: '',
      usedNormalisedMatch: normalised,
    );
  }

  return PatchResult(
    ok: true,
    text: scratch,
    applied: applied,
    errors: const [],
    diff: unifiedDiff(source, scratch),
    usedNormalisedMatch: normalised,
  );
}

List<int> _findAll(String hay, String needle) {
  final out = <int>[];
  var from = 0;
  while (true) {
    final at = hay.indexOf(needle, from);
    if (at < 0) return out;
    out.add(at);
    from = at + 1;
    if (out.length > 32) return out; // enough to call it ambiguous
  }
}

/// Line-wise match ignoring CRLF and trailing whitespace. Returns character
/// ranges in the ORIGINAL text so the replacement lands byte-correctly.
List<(int, int)> _findNormalised(String hay, String needle) {
  final hayLines = hay.split('\n');
  final needleLines = needle.replaceAll('\r\n', '\n').split('\n');
  if (needleLines.isEmpty) return const [];

  // Character offset of the start of each line.
  final offsets = <int>[];
  var off = 0;
  for (final l in hayLines) {
    offsets.add(off);
    off += l.length + 1;
  }

  String norm(String s) => s.replaceAll('\r', '').trimRight();

  final out = <(int, int)>[];
  for (var i = 0; i + needleLines.length <= hayLines.length; i++) {
    var hit = true;
    for (var j = 0; j < needleLines.length; j++) {
      if (norm(hayLines[i + j]) != norm(needleLines[j])) {
        hit = false;
        break;
      }
    }
    if (!hit) continue;
    final start = offsets[i];
    final lastIdx = i + needleLines.length - 1;
    final end = offsets[lastIdx] + hayLines[lastIdx].length;
    out.add((start, end));
    if (out.length > 8) break;
  }
  return out;
}

int _lineOf(String s, int offset) =>
    '\n'.allMatches(s.substring(0, offset)).length + 1;

/// Nearest lines to the first substantive SEARCH line, by bigram similarity.
List<PatchCandidate> _candidates(String source, String search) {
  final target = search
      .split('\n')
      .firstWhere((l) => l.trim().isNotEmpty, orElse: () => '')
      .trim();
  if (target.isEmpty) return const [];

  final lines = source.split('\n');
  final scored = <PatchCandidate>[];
  for (var i = 0; i < lines.length; i++) {
    final sim = _similarity(target, lines[i].trim());
    if (sim >= 0.55) {
      scored.add(PatchCandidate(i + 1, lines[i].trimRight(), sim));
    }
  }
  scored.sort((a, b) => b.similarity.compareTo(a.similarity));
  return scored.take(3).toList();
}

/// Dice coefficient over character bigrams — cheap, dependency-free, and only
/// ever used to SUGGEST, never to decide a match.
double _similarity(String a, String b) {
  if (a == b) return 1;
  if (a.length < 2 || b.length < 2) return 0;
  final pairs = <String, int>{};
  for (var i = 0; i < a.length - 1; i++) {
    final k = a.substring(i, i + 2);
    pairs[k] = (pairs[k] ?? 0) + 1;
  }
  var hits = 0;
  for (var i = 0; i < b.length - 1; i++) {
    final k = b.substring(i, i + 2);
    final n = pairs[k] ?? 0;
    if (n > 0) {
      pairs[k] = n - 1;
      hits++;
    }
  }
  return 2 * hits / (a.length - 1 + b.length - 1);
}

// ---------------------------------------------------------------------------
// Unified diff
// ---------------------------------------------------------------------------

/// Line-based unified diff. Used both as the `apply_patch` tool result and as
/// the review surface in the confirmation card, so it must stay readable —
/// hence the [maxLines] cap.
String unifiedDiff(String a, String b, {int context = 3, int maxLines = 400}) {
  final al = a.split('\n');
  final bl = b.split('\n');

  // Trim the common head/tail first: for a 550-line file with a 2-line edit
  // this keeps the LCS table tiny.
  var head = 0;
  while (head < al.length && head < bl.length && al[head] == bl[head]) {
    head++;
  }
  var tail = 0;
  while (tail < al.length - head &&
      tail < bl.length - head &&
      al[al.length - 1 - tail] == bl[bl.length - 1 - tail]) {
    tail++;
  }
  final aMid = al.sublist(head, al.length - tail);
  final bMid = bl.sublist(head, bl.length - tail);
  if (aMid.isEmpty && bMid.isEmpty) return '';

  final ops = (aMid.length * bMid.length > 4000000)
      ? [
          for (final l in aMid) ('-', l),
          for (final l in bMid) ('+', l),
        ]
      : _lcsOps(aMid, bMid);

  final out = <String>[];
  out.add('@@ -${head + 1},${aMid.length} +${head + 1},${bMid.length} @@');
  // Context lines around the change, from the trimmed head/tail.
  for (var i = math.max(0, head - context); i < head; i++) {
    out.add(' ${al[i]}');
  }
  for (final (sign, line) in ops) {
    out.add('$sign$line');
    if (out.length >= maxLines) {
      out.add('… diff truncated (${ops.length} changed lines total)');
      return out.join('\n');
    }
  }
  final tailStart = al.length - tail;
  for (var i = tailStart; i < math.min(al.length, tailStart + context); i++) {
    out.add(' ${al[i]}');
  }
  return out.join('\n');
}

List<(String, String)> _lcsOps(List<String> a, List<String> b) {
  final n = a.length, m = b.length;
  final dp = List.generate(n + 1, (_) => List<int>.filled(m + 1, 0));
  for (var i = n - 1; i >= 0; i--) {
    for (var j = m - 1; j >= 0; j--) {
      dp[i][j] = a[i] == b[j]
          ? dp[i + 1][j + 1] + 1
          : math.max(dp[i + 1][j], dp[i][j + 1]);
    }
  }
  final out = <(String, String)>[];
  var i = 0, j = 0;
  while (i < n && j < m) {
    if (a[i] == b[j]) {
      out.add((' ', a[i]));
      i++;
      j++;
    } else if (dp[i + 1][j] >= dp[i][j + 1]) {
      out.add(('-', a[i]));
      i++;
    } else {
      out.add(('+', b[j]));
      j++;
    }
  }
  while (i < n) {
    out.add(('-', a[i++]));
  }
  while (j < m) {
    out.add(('+', b[j++]));
  }
  return out;
}
