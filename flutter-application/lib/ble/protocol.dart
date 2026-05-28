/// Binary TLV + chunking codec for the NotifBridge BLE link.
///
/// A single logical message is wrapped in PDU bytes then sliced into BLE
/// chunks. Each chunk has an 8-byte header so the firmware can reassemble
/// without buffering more than one in-flight message per direction.
///
/// Chunk header (little-endian):
///   u8  type       — 1 NOTIF, 2 MEDIA, 3 ICON, 4 ALBUM_ART,
///                    5 CMD,   6 ACK,   7 KEEPALIVE
///   u8  flags      — bit0 START, bit1 END
///   u16 seq        — per-direction monotonic
///   u32 total_len  — only meaningful in the START chunk
library;

import 'dart:typed_data';

enum PduType {
  notification(1),
  media(2),
  icon(3),
  albumArt(4),
  command(5),
  ack(6),
  keepalive(7);

  final int code;
  const PduType(this.code);
  static PduType fromCode(int c) =>
      PduType.values.firstWhere((e) => e.code == c, orElse: () => PduType.ack);
}

class ChunkFlags {
  static const int start = 0x01;
  static const int end = 0x02;
}

const int kChunkHeaderLen = 8;
// Conservative: MTU 247 - 3 (ATT) - 8 (header) = 236.
const int kMaxChunkPayload = 236;

class ChunkedEncoder {
  int _seq = 0;

  /// Slice a complete PDU body into BLE-sized chunks (already including header).
  List<Uint8List> encode(PduType type, Uint8List body) {
    final seq = _seq++ & 0xffff;
    final total = body.length;
    if (total == 0) {
      return [_header(type, ChunkFlags.start | ChunkFlags.end, seq, 0)];
    }
    final chunks = <Uint8List>[];
    int off = 0;
    while (off < total) {
      final remaining = total - off;
      final take = remaining > kMaxChunkPayload ? kMaxChunkPayload : remaining;
      final isFirst = off == 0;
      final isLast = (off + take) == total;
      int flags = 0;
      if (isFirst) flags |= ChunkFlags.start;
      if (isLast) flags |= ChunkFlags.end;
      final hdr = _header(type, flags, seq, isFirst ? total : 0);
      final out = Uint8List(kChunkHeaderLen + take)
        ..setRange(0, kChunkHeaderLen, hdr)
        ..setRange(kChunkHeaderLen, kChunkHeaderLen + take, body, off);
      chunks.add(out);
      off += take;
    }
    return chunks;
  }

  Uint8List _header(PduType t, int flags, int seq, int totalLen) {
    final b = ByteData(kChunkHeaderLen);
    b.setUint8(0, t.code);
    b.setUint8(1, flags);
    b.setUint16(2, seq, Endian.little);
    b.setUint32(4, totalLen, Endian.little);
    return b.buffer.asUint8List();
  }
}

/// Reassembles inbound chunks (P4 → phone). The phone only listens for
/// CMD/ACK/KEEPALIVE/icon-requests, so the buffer is small.
class ChunkedDecoder {
  PduType? _type;
  int? _seq;
  final BytesBuilder _buf = BytesBuilder();
  int _expected = 0;

  /// Returns a complete PDU body when the END chunk lands, else null.
  ({PduType type, Uint8List body})? feed(Uint8List chunk) {
    if (chunk.length < kChunkHeaderLen) return null;
    final h = ByteData.sublistView(chunk, 0, kChunkHeaderLen);
    final type = PduType.fromCode(h.getUint8(0));
    final flags = h.getUint8(1);
    final seq = h.getUint16(2, Endian.little);
    final totalLen = h.getUint32(4, Endian.little);
    final isStart = (flags & ChunkFlags.start) != 0;
    final isEnd = (flags & ChunkFlags.end) != 0;

    if (isStart) {
      _type = type;
      _seq = seq;
      _expected = totalLen;
      _buf.clear();
    } else if (_seq != seq || _type != type) {
      _reset();
      return null;
    }
    _buf.add(chunk.sublist(kChunkHeaderLen));
    if (!isEnd) return null;
    final body = _buf.takeBytes();
    final out = (type: _type!, body: body);
    _reset();
    if (out.body.length != _expected && _expected != 0) {
      // Length mismatch — drop silently; firmware retries.
      return null;
    }
    return out;
  }

  void _reset() {
    _type = null;
    _seq = null;
    _expected = 0;
    _buf.clear();
  }
}

/// TLV codec inside a PDU body.
///   u8  tag
///   u32 len (LE)
///   bytes value (len bytes)
class Tlv {
  static Uint8List encode(Map<int, Object> fields) {
    final out = BytesBuilder();
    fields.forEach((tag, value) {
      final v = _toBytes(value);
      final h = ByteData(5)
        ..setUint8(0, tag)
        ..setUint32(1, v.length, Endian.little);
      out.add(h.buffer.asUint8List());
      out.add(v);
    });
    return out.takeBytes();
  }

  static Map<int, Uint8List> decode(Uint8List body) {
    final out = <int, Uint8List>{};
    int off = 0;
    while (off + 5 <= body.length) {
      final tag = body[off];
      final len = ByteData.sublistView(body, off + 1, off + 5)
          .getUint32(0, Endian.little);
      off += 5;
      if (off + len > body.length) break;
      out[tag] = Uint8List.sublistView(body, off, off + len);
      off += len;
    }
    return out;
  }

  static Uint8List _toBytes(Object v) {
    if (v is Uint8List) return v;
    if (v is List<int>) return Uint8List.fromList(v);
    if (v is String) {
      return Uint8List.fromList(
          v.codeUnits.isEmpty ? const <int>[] : _utf8(v));
    }
    if (v is int) {
      // Default int → u32 LE.
      final b = ByteData(4)..setUint32(0, v, Endian.little);
      return b.buffer.asUint8List();
    }
    if (v is bool) return Uint8List.fromList([v ? 1 : 0]);
    throw ArgumentError('TLV: unsupported value type ${v.runtimeType}');
  }

  static List<int> _utf8(String s) {
    // Avoid importing dart:convert at top level to keep this file standalone.
    return s.codeUnits.expand((cu) {
      if (cu < 0x80) return [cu];
      if (cu < 0x800) return [0xc0 | (cu >> 6), 0x80 | (cu & 0x3f)];
      return [
        0xe0 | (cu >> 12),
        0x80 | ((cu >> 6) & 0x3f),
        0x80 | (cu & 0x3f),
      ];
    }).toList(growable: false);
  }
}

/// Convenience reader.
class TlvReader {
  final Map<int, Uint8List> fields;
  TlvReader(this.fields);

  String? str(int tag) {
    final b = fields[tag];
    if (b == null) return null;
    return String.fromCharCodes(b);
  }

  int? u32(int tag) {
    final b = fields[tag];
    if (b == null || b.length < 4) return null;
    return ByteData.sublistView(b).getUint32(0, Endian.little);
  }

  bool? boolean(int tag) {
    final b = fields[tag];
    if (b == null || b.isEmpty) return null;
    return b[0] != 0;
  }
}
