/// Tests for the VESC console channel: the print-payload parser and the ring
/// that buffers it. Both run without hardware and without a Flutter binding.
library;

import 'dart:typed_data';

import 'package:aa_bridge/ble/vesc/lisp_console_ring.dart';
import 'package:aa_bridge/ble/vesc/vesc_commands.dart';
import 'package:flutter_test/flutter_test.dart';

Uint8List printPayload(List<int> bodyBytes, {int cmd = VescCmd.lispPrint}) =>
    Uint8List.fromList([cmd, ...bodyBytes]);

Uint8List printText(String s, {int cmd = VescCmd.lispPrint}) =>
    printPayload(s.codeUnits, cmd: cmd);

void main() {
  group('parseVescPrint', () {
    test('plain text with no NUL terminator', () {
      expect(parseVescPrint(printText('agent-probe')), ['agent-probe']);
    });

    test('trailing NUL, CR and LF are stripped', () {
      expect(parseVescPrint(printPayload('hi'.codeUnits + [0x0d, 0x0a, 0x00])),
          ['hi']);
    });

    test('one packet can carry several lines', () {
      expect(parseVescPrint(printText('a\nb\nc')), ['a', 'b', 'c']);
    });

    test('blank lines are skipped, not emitted as empty output', () {
      expect(parseVescPrint(printText('a\n\n\nb')), ['a', 'b']);
    });

    test('COMM_PRINT is accepted the same way', () {
      expect(parseVescPrint(printText('x', cmd: VescCmd.commPrint)), ['x']);
    });

    test('malformed UTF-8 does not throw', () {
      final p = printPayload([0xff, 0xfe, 0x41]);
      expect(() => parseVescPrint(p), returnsNormally);
      expect(parseVescPrint(p).single, endsWith('A'));
    });

    test('empty and command-only payloads yield nothing', () {
      expect(parseVescPrint(Uint8List(0)), isEmpty);
      expect(parseVescPrint(printPayload(const [])), isEmpty);
      expect(parseVescPrint(printPayload(const [0, 0, 0])), isEmpty);
    });

    test('an over-long line is capped', () {
      final long = 'x' * 900;
      expect(parseVescPrint(printText(long)).single.length, 512);
    });
  });

  group('LispConsoleRing', () {
    test('seq is monotonic and survives clear()', () {
      final r = LispConsoleRing();
      r.add('a');
      r.add('b');
      r.clear();
      r.add('c');
      // A reader holding an old cursor must not re-read cleared content.
      expect(r.read(sinceSeq: 0).lines.single.text, 'c');
      expect(r.read(sinceSeq: 0).lines.single.seq, 2);
    });

    test('evicts by line count and counts the drops', () {
      final r = LispConsoleRing(maxLines: 3);
      for (var i = 0; i < 10; i++) {
        r.add('line$i');
      }
      final chunk = r.read();
      expect(chunk.lines.length, 3);
      expect(chunk.lines.first.text, 'line7');
      expect(chunk.dropped, 7);
    });

    test('evicts by total bytes even when the line count is fine', () {
      final r = LispConsoleRing(maxLines: 1000, maxBytes: 100);
      for (var i = 0; i < 20; i++) {
        r.add('y' * 30);
      }
      final chunk = r.read();
      expect(chunk.lines.length, lessThanOrEqualTo(4));
      expect(chunk.dropped, greaterThan(0));
    });

    test('read() resumes from a cursor and reports the next one', () {
      final r = LispConsoleRing();
      for (var i = 0; i < 5; i++) {
        r.add('l$i');
      }
      final first = r.read(maxLines: 2);
      expect([for (final l in first.lines) l.text], ['l0', 'l1']);
      final second = r.read(sinceSeq: first.nextSeq, maxLines: 2);
      expect([for (final l in second.lines) l.text], ['l2', 'l3']);
    });

    test('alive only flips on real print output, not markers', () {
      final r = LispConsoleRing();
      r.add('— link rebound —', kind: 'marker');
      expect(r.read().alive, isFalse);
      r.add('agent-probe');
      expect(r.read().alive, isTrue);
    });

    test('addRaw dumps a bounded hex preview', () {
      final r = LispConsoleRing();
      r.addRaw(4, Uint8List.fromList(List.generate(64, (i) => i)));
      final line = r.read().lines.single;
      expect(line.kind, 'raw');
      expect(line.text, startsWith('cmd 4, 64 B: 00 01 02'));
      expect(line.text, endsWith('…'));
    });
  });
}
