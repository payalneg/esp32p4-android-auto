/// The PBF reader is the only piece here that decodes someone else's binary
/// format by hand, so its edges are pinned: the wire format itself, and the
/// evaluation-order trap that made skipping a field land short.
library;

import 'dart:convert';
import 'dart:io';
import 'dart:typed_data';

import 'package:aa_bridge/nav/pbf.dart';
import 'package:flutter_test/flutter_test.dart';

/// Protobuf varint, little-endian base-128.
List<int> varint(int v) {
  final out = <int>[];
  var x = v;
  while (x >= 0x80) {
    out.add((x & 0x7F) | 0x80);
    x >>= 7;
  }
  out.add(x);
  return out;
}

List<int> tag(int field, int wire) => varint((field << 3) | wire);

List<int> lengthDelimited(int field, List<int> body) =>
    <int>[...tag(field, 2), ...varint(body.length), ...body];

void main() {
  group('ProtoReader', () {
    test('reads varints of every width', () {
      for (final v in <int>[0, 1, 127, 128, 300, 16384, 1 << 32, 1 << 62]) {
        final r = ProtoReader(Uint8List.fromList(varint(v)));
        expect(r.readVarint(), v, reason: '$v');
      }
    });

    test('zig-zag decodes negatives', () {
      // sint64: -1 encodes as 1, 1 as 2, -2 as 3.
      expect(ProtoReader(Uint8List.fromList(varint(1))).readSVarint(), -1);
      expect(ProtoReader(Uint8List.fromList(varint(2))).readSVarint(), 1);
      expect(ProtoReader(Uint8List.fromList(varint(3))).readSVarint(), -2);
      expect(ProtoReader(Uint8List.fromList(varint(0))).readSVarint(), 0);
    });

    test('splits a tag into field and wire type', () {
      final r = ProtoReader(Uint8List.fromList(tag(8, 2)));
      expect(r.readTag(), (8, 2));
    });

    test('skipping a length-delimited field lands exactly past it', () {
      // The bug this pins: `pos += readVarint()` adds the length to the
      // position from before the length itself was read, stopping short by
      // the width of that varint and desynchronising everything after.
      final body = List<int>.filled(200, 0x42);
      final bytes = Uint8List.fromList(<int>[
        ...lengthDelimited(2, body),
        ...tag(3, 0),
        ...varint(7),
      ]);
      final r = ProtoReader(bytes);
      final (field, wire) = r.readTag();
      expect((field, wire), (2, 2));
      r.skip(wire);
      // Whatever follows must still parse: that is what "exactly past" means.
      expect(r.readTag(), (3, 0));
      expect(r.readVarint(), 7);
      expect(r.isDone, isTrue);
    });

    test('skips the fixed-width and varint kinds too', () {
      final bytes = Uint8List.fromList(<int>[
        ...tag(1, 0), ...varint(300),
        ...tag(2, 5), 1, 2, 3, 4,
        ...tag(3, 1), 1, 2, 3, 4, 5, 6, 7, 8,
        ...tag(4, 0), ...varint(9),
      ]);
      final r = ProtoReader(bytes);
      for (var i = 0; i < 3; i++) {
        final (_, wire) = r.readTag();
        r.skip(wire);
      }
      expect(r.readTag(), (4, 0));
      expect(r.readVarint(), 9);
    });

    test('refuses a truncated varint rather than guessing', () {
      final r = ProtoReader(Uint8List.fromList(<int>[0x80, 0x80]));
      expect(r.readVarint, throwsFormatException);
    });

    test('refuses a length that runs past the end', () {
      final r = ProtoReader(Uint8List.fromList(<int>[...varint(50), 1, 2]));
      expect(r.readBytes, throwsFormatException);
    });

    test('an unknown wire type is an error, not a silent skip', () {
      final r = ProtoReader(Uint8List.fromList(<int>[0]));
      expect(() => r.skip(7), throwsFormatException);
    });
  });

  group('PbfFile', () {
    late Directory dir;

    setUp(() => dir = Directory.systemTemp.createTempSync('pbf'));
    tearDown(() => dir.deleteSync(recursive: true));

    /// Wraps [payload] as one blob, the way a real file does.
    List<int> blob(String type, List<int> payload, {bool compress = true}) {
      final data = compress
          ? lengthDelimited(3, ZLibCodec().encode(payload))
          : lengthDelimited(1, payload);
      final header = <int>[
        ...lengthDelimited(1, utf8.encode(type)),
        ...tag(3, 0),
        ...varint(data.length),
      ];
      return <int>[
        (header.length >> 24) & 0xFF,
        (header.length >> 16) & 0xFF,
        (header.length >> 8) & 0xFF,
        header.length & 0xFF,
        ...header,
        ...data,
      ];
    }

    /// A PrimitiveBlock with a string table, one group and non-default scaling.
    List<int> primitiveBlock() => <int>[
          ...lengthDelimited(
              1,
              <int>[
                ...lengthDelimited(1, utf8.encode('')),
                ...lengthDelimited(1, utf8.encode('highway')),
                ...lengthDelimited(1, utf8.encode('residential')),
              ]),
          ...lengthDelimited(2, <int>[...tag(3, 2), ...varint(0)]),
          ...tag(17, 0), ...varint(100),
          ...tag(19, 0), ...varint(1000),
          ...tag(20, 0), ...varint(2000),
        ];

    test('walks blobs, inflates them and reads the header fields', () async {
      final file = File('${dir.path}/a.pbf');
      file.writeAsBytesSync(<int>[
        ...blob('OSMHeader', <int>[...tag(1, 0), ...varint(1)]),
        ...blob('OSMData', primitiveBlock()),
        ...blob('OSMData', primitiveBlock()),
      ]);

      final blocks = <PrimitiveBlock>[];
      await PbfFile(file.path).forEachBlock(blocks.add);

      expect(blocks, hasLength(2), reason: 'the header blob is not data');
      final b = blocks.first;
      expect(b.strings, <String>['', 'highway', 'residential']);
      expect(b.groups, hasLength(1));
      expect(b.granularity, 100);
      expect(b.lat(0), closeTo(1000 * 1e-9, 1e-15));
      expect(b.lon(10), closeTo((2000 + 100 * 10) * 1e-9, 1e-15));
    });

    test('reads an uncompressed blob as well', () async {
      final file = File('${dir.path}/raw.pbf');
      file.writeAsBytesSync(blob('OSMData', primitiveBlock(), compress: false));
      final blocks = <PrimitiveBlock>[];
      await PbfFile(file.path).forEachBlock(blocks.add);
      expect(blocks, hasLength(1));
      expect(blocks.single.strings, contains('highway'));
    });

    test('reports progress and honours cancellation', () async {
      final file = File('${dir.path}/many.pbf');
      file.writeAsBytesSync(<int>[
        for (var i = 0; i < 5; i++) ...blob('OSMData', primitiveBlock()),
      ]);

      var seen = 0;
      final reads = <int>[];
      await PbfFile(file.path).forEachBlock(
        (_) => seen++,
        onProgress: (read, total) => reads.add(read),
        cancelled: () => seen >= 2,
      );
      expect(seen, 2);
      expect(reads, isNotEmpty);
      expect(reads.last, lessThanOrEqualTo(file.lengthSync()));
    });
  });
}
