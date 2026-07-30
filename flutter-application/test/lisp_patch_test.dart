/// Patch engine tests. The failure paths matter as much as the happy one:
/// every rejection here becomes a `tool` message the model has to self-correct
/// from, so it must be specific enough to act on.
library;

import 'dart:io';

import 'package:aa_bridge/agent/lisp_patch.dart';
import 'package:flutter_test/flutter_test.dart';

String block(String search, String replace) => '<<<<<<< SEARCH\n'
    '$search\n'
    '=======\n'
    '$replace\n'
    '>>>>>>> REPLACE';

String get _mainLisp {
  for (final p in ['../lisp/main.lisp', 'lisp/main.lisp']) {
    final f = File(p);
    if (f.existsSync()) return f.readAsStringSync();
  }
  fail('lisp/main.lisp not found');
}

void main() {
  group('parsePatch', () {
    test('reads a single block', () {
      final b = parsePatch(block('a', 'b'));
      expect(b, hasLength(1));
      expect(b.single.search, 'a');
      expect(b.single.replace, 'b');
    });

    test('reads several blocks with prose between them', () {
      final raw = 'First I will bump the gain:\n'
          '${block('(def kp 1)', '(def kp 2)')}\n'
          'and then add the log line:\n'
          '${block('(foo)', '(foo)\n(print "hi")')}\n';
      expect(parsePatch(raw), hasLength(2));
    });

    test('tolerates markdown fences and CRLF', () {
      final raw = '```\r\n${block('a', 'b').replaceAll('\n', '\r\n')}\r\n```';
      final b = parsePatch(raw);
      expect(b, hasLength(1));
      expect(b.single.search, 'a');
    });

    test('an unterminated block yields nothing', () {
      expect(parsePatch('<<<<<<< SEARCH\nfoo\n'), isEmpty);
      expect(parsePatch('<<<<<<< SEARCH\nfoo\n=======\nbar\n'), isEmpty);
    });
  });

  group('applyPatch', () {
    const src = 'line one\nline two\nline three\n';

    test('applies a unique match and reports a diff', () {
      final r = applyPatch(src, block('line two', 'line 2'));
      expect(r.ok, isTrue);
      expect(r.applied, 1);
      expect(r.text, 'line one\nline 2\nline three\n');
      expect(r.diff, contains('-line two'));
      expect(r.diff, contains('+line 2'));
    });

    test('applies several blocks in order', () {
      final r = applyPatch(
          src, '${block('line one', 'first')}\n${block('line three', 'last')}');
      expect(r.ok, isTrue);
      expect(r.applied, 2);
      expect(r.text, 'first\nline two\nlast\n');
    });

    test('is transactional — one bad block discards the whole patch', () {
      final r = applyPatch(
          src, '${block('line one', 'first')}\n${block('nope', 'x')}');
      expect(r.ok, isFalse);
      expect(r.text, isNull);
      expect(r.applied, 0);
      expect(r.errors.map((e) => e.kind), contains(PatchErrorKind.noMatch));
    });

    test('rejects an ambiguous SEARCH and says where it matched', () {
      final r = applyPatch('(foo)\n(bar)\n(foo)\n', block('(foo)', '(baz)'));
      expect(r.ok, isFalse);
      final e = r.errors.single;
      expect(e.kind, PatchErrorKind.ambiguous);
      expect(e.matchLines, [1, 3]);
    });

    test('rejects an empty SEARCH', () {
      final r = applyPatch(src, block('', 'appended'));
      expect(r.ok, isFalse);
      expect(r.errors.single.kind, PatchErrorKind.emptySearch);
      // The reason matters: blind appends land after @const-end.
      expect(r.errors.single.message, contains('@const-end'));
    });

    test('rejects a patch with no blocks at all', () {
      final r = applyPatch(src, 'I think you should change line two.');
      expect(r.ok, isFalse);
      expect(r.errors.single.kind, PatchErrorKind.malformed);
    });

    test('flags an opened-but-unclosed block as malformed', () {
      final r = applyPatch(src, '<<<<<<< SEARCH\nline two\n=======\nx\n');
      expect(r.ok, isFalse);
      expect(r.errors.single.kind, PatchErrorKind.malformed);
      expect(r.errors.single.message, contains('never closed'));
    });

    test('offers near-miss candidates when SEARCH is not found', () {
      final r = applyPatch(
          '(def cruise-kp 0.02)  ; cruise PI gain\n', block('(def cruise-kp 0.2)', 'x'));
      expect(r.ok, isFalse);
      final e = r.errors.single;
      expect(e.kind, PatchErrorKind.noMatch);
      expect(e.candidates, isNotEmpty);
      expect(e.candidates.first.line, 1);
      expect(e.candidates.first.similarity, greaterThan(0.55));
    });

    test('forgives CRLF and trailing whitespace, and says it did', () {
      final crlf = 'alpha\r\nbeta   \r\ngamma\r\n';
      final r = applyPatch(crlf, block('alpha\nbeta', 'ALPHA\nBETA'));
      expect(r.ok, isTrue, reason: r.errors.map((e) => e.message).join());
      expect(r.usedNormalisedMatch, isTrue);
      expect(r.text, contains('ALPHA'));
    });

    test('does not fuzzy-match on content', () {
      // One character off, and nothing else close by: must NOT be applied.
      final r = applyPatch('(def kp 0.02)\n', block('(def kp 0.03)', 'x'));
      expect(r.ok, isFalse);
    });

    test('enforces the size limit', () {
      final r = applyPatch(src, block('line two', 'x' * 200), maxBytes: 100);
      expect(r.ok, isFalse);
      expect(r.errors.single.kind, PatchErrorKind.tooLarge);
    });

    test('edits the real main.lisp', () {
      final r = applyPatch(
          _mainLisp,
          block('(def cruise-kp 0.02)  ; cruise PI: A per ERPM of error',
              '(def cruise-kp 0.035) ; cruise PI: A per ERPM of error'));
      expect(r.ok, isTrue, reason: r.errors.map((e) => e.message).join());
      expect(r.text, contains('(def cruise-kp 0.035)'));
      expect(r.text, isNot(contains('(def cruise-kp 0.02)')));
      // Everything else survives.
      expect(r.text!.split('\n').length, _mainLisp.split('\n').length);
    });
  });

  group('unifiedDiff', () {
    test('is empty for identical input', () {
      expect(unifiedDiff('a\nb\n', 'a\nb\n'), '');
    });

    test('shows an insertion with context', () {
      final d = unifiedDiff('a\nb\nc\n', 'a\nb\nX\nc\n');
      expect(d, contains('+X'));
      expect(d, contains(' b'));
    });

    test('shows a deletion', () {
      expect(unifiedDiff('a\nb\nc\n', 'a\nc\n'), contains('-b'));
    });

    test('truncates a huge diff instead of dumping it', () {
      final a = List.generate(1000, (i) => 'old $i').join('\n');
      final b = List.generate(1000, (i) => 'new $i').join('\n');
      final d = unifiedDiff(a, b, maxLines: 50);
      expect(d.split('\n').length, lessThanOrEqualTo(52));
      expect(d, contains('truncated'));
    });

    test('handles a small edit in a big file quickly', () {
      final big = _mainLisp;
      final edited = big.replaceFirst('(def cruise-kp 0.02)', '(def cruise-kp 0.9)');
      final sw = Stopwatch()..start();
      final d = unifiedDiff(big, edited);
      sw.stop();
      expect(d, contains('+(def cruise-kp 0.9)'));
      expect(sw.elapsedMilliseconds, lessThan(500));
    });
  });
}
