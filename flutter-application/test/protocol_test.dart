import 'dart:convert';
import 'dart:typed_data';

import 'package:flutter_test/flutter_test.dart';

import 'package:aa_bridge/ble/protocol.dart';

void main() {
  group('Tlv string encoding', () {
    test('ASCII round-trips', () {
      final fields = Tlv.decode(Tlv.encode({1: '/vescfs/main.lisp'}));
      expect(TlvReader(fields).str(1), '/vescfs/main.lisp');
    });

    test('Cyrillic round-trips as UTF-8', () {
      const path = '/sdcard/Музыка/трек.mp3';
      final bytes = Tlv.decode(Tlv.encode({1: path}))[1]!;
      // Wire bytes are real UTF-8, not one-byte-per-code-unit.
      expect(bytes, utf8.encode(path));
      expect(TlvReader(Tlv.decode(Tlv.encode({1: path}))).str(1), path);
    });

    test('emoji survive: no CESU-8 surrogate halves', () {
      // The old hand-rolled encoder emitted 3 bytes per UTF-16 surrogate (6
      // total for one emoji) instead of the correct 4-byte sequence.
      const s = 'ok 🚴 go';
      final bytes = Tlv.decode(Tlv.encode({7: s}))[7]!;
      expect(bytes, utf8.encode(s));
      expect(TlvReader(Tlv.decode(Tlv.encode({7: s}))).str(7), s);
      // Nothing in range U+D800..U+DFFF got encoded (0xED leads those).
      expect(bytes.contains(0xED), isFalse);
    });

    test('empty string encodes to a zero-length field', () {
      final fields = Tlv.decode(Tlv.encode({3: ''}));
      expect(fields[3], isEmpty);
      expect(TlvReader(fields).str(3), '');
    });

    test('malformed UTF-8 on read does not throw', () {
      final reader = TlvReader({5: Uint8List.fromList([0xff, 0xfe, 0x41])});
      expect(reader.str(5), endsWith('A'));
    });
  });

  group('ChunkedDecoder', () {
    test('reassembles a multi-chunk body', () {
      final body = Uint8List.fromList(
          List<int>.generate(600, (i) => i & 0xff));
      final chunks = ChunkedEncoder().encode(PduType.icon, body);
      expect(chunks.length, greaterThan(1));

      final dec = ChunkedDecoder();
      final results = chunks.map(dec.feed).toList();
      // Only the END chunk yields a PDU.
      expect(results.sublist(0, results.length - 1), everyElement(isNull));
      final out = results.last!;
      expect(out.type, PduType.icon);
      expect(out.body, body);
    });

    test('single START|END chunk yields its body', () {
      final body = Uint8List.fromList([1, 2, 3]);
      final chunks = ChunkedEncoder().encode(PduType.command, body);
      expect(chunks.length, 1);
      final out = ChunkedDecoder().feed(chunks.single)!;
      expect(out.type, PduType.command);
      expect(out.body, body);
    });

    test('an empty body is delivered, not dropped', () {
      // total_len 0 means "unknown/empty" — the length check must not fire.
      final chunks = ChunkedEncoder().encode(PduType.ack, Uint8List(0));
      final out = ChunkedDecoder().feed(chunks.single)!;
      expect(out.type, PduType.ack);
      expect(out.body, isEmpty);
    });

    test('a body shorter than the announced total_len is dropped', () {
      // This check used to be dead: _reset() zeroed _expected before the
      // comparison, so its `!= 0` guard was always false and truncated PDUs
      // were handed upstream.
      final body = Uint8List.fromList(List<int>.filled(300, 0xaa));
      final chunks = ChunkedEncoder().encode(PduType.icon, body);
      expect(chunks.length, 2);
      // Drop the tail of the last chunk, keeping its END flag.
      final short = chunks[1].sublist(0, kChunkHeaderLen + 1);

      final dec = ChunkedDecoder();
      expect(dec.feed(chunks[0]), isNull);
      expect(dec.feed(short), isNull);
    });

    test('a chunk from a different seq resets the assembly', () {
      final enc = ChunkedEncoder();
      final first = enc.encode(PduType.icon,
          Uint8List.fromList(List<int>.filled(300, 1)));
      final second = enc.encode(PduType.icon,
          Uint8List.fromList(List<int>.filled(300, 2)));

      final dec = ChunkedDecoder();
      expect(dec.feed(first[0]), isNull);
      // The END chunk of a different message must not complete the first one.
      expect(dec.feed(second[1]), isNull);
      // ...and the decoder is usable again afterwards.
      final third = enc.encode(PduType.command, Uint8List.fromList([9]));
      expect(dec.feed(third.single)!.body, [9]);
    });
  });
}
