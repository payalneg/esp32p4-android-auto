/// VESC packet framing + reassembly, ported from VESC Tool `packet.cpp`.
///
/// Frame: [start][len][payload][crc16][end=3]
///   start 2 → 1 length byte      (payload ≤ 255)
///   start 3 → 2 length bytes      (payload ≤ 65535)
///   start 4 → 3 length bytes      (payload larger)
/// crc16 is over the payload only. The head unit's NUS bridge speaks this exact
/// framing (it forwards the payload straight to the VESC over CAN).
library;

import 'dart:typed_data';

import 'crc16.dart';

class VescPacket {
  /// Frame [payload] into a wire packet ready to write to NUS RX.
  static Uint8List encode(Uint8List payload) {
    final len = payload.length;
    final b = BytesBuilder();
    if (len <= 255) {
      b.addByte(2);
      b.addByte(len);
    } else if (len <= 65535) {
      b.addByte(3);
      b.addByte((len >> 8) & 0xFF);
      b.addByte(len & 0xFF);
    } else {
      b.addByte(4);
      b.addByte((len >> 16) & 0xFF);
      b.addByte((len >> 8) & 0xFF);
      b.addByte(len & 0xFF);
    }
    final crc = crc16(payload);
    b.add(payload);
    b.addByte((crc >> 8) & 0xFF);
    b.addByte(crc & 0xFF);
    b.addByte(3);
    return b.toBytes();
  }
}

/// Reassembles complete VESC packet payloads out of the raw byte stream that
/// arrives over the NUS TX notifications (a packet may span several
/// notifications, or several packets may arrive in one). Mirrors the resync
/// logic of `Packet::processData` / `try_decode_packet`.
class VescPacketDecoder {
  final List<int> _buf = [];
  final int maxLen;

  VescPacketDecoder({this.maxLen = 512 * 1024});

  List<Uint8List> feed(Uint8List data) {
    _buf.addAll(data);
    final out = <Uint8List>[];
    while (_buf.isNotEmpty) {
      final (consumed, payload) = _tryDecode();
      if (consumed == 0) break; // need more data
      if (consumed < 0) {
        _buf.removeAt(0); // bad start byte — resync
        continue;
      }
      if (payload != null) out.add(payload);
      _buf.removeRange(0, consumed);
    }
    // Cap runaway buffering if a stream never resolves.
    if (_buf.length > maxLen + 16) _buf.clear();
    return out;
  }

  /// Returns (bytesConsumed, payload):
  ///  consumed 0  → need more data (leave buffer intact)
  ///  consumed <0 → invalid start, caller drops one byte
  ///  consumed >0 → a full packet was consumed; payload set on CRC match
  (int, Uint8List?) _tryDecode() {
    if (_buf.isEmpty) return (0, null);
    final start = _buf[0];
    if (start != 2 && start != 3 && start != 4) return (-1, null);
    final dataStart = start; // number of header bytes (start + length bytes)
    if (_buf.length < dataStart) return (0, null);

    int len;
    if (start == 2) {
      len = _buf[1];
      if (len < 1) return (-1, null);
    } else if (start == 3) {
      len = (_buf[1] << 8) | _buf[2];
      if (len < 255) return (-1, null);
    } else {
      len = (_buf[1] << 16) | (_buf[2] << 8) | _buf[3];
      if (len < 65535) return (-1, null);
    }
    if (len > maxLen) return (-1, null);

    final total = len + dataStart + 3; // header + payload + crc(2) + end(1)
    if (_buf.length < total) return (0, null);
    if (_buf[dataStart + len + 2] != 3) return (-1, null);

    final payload =
        Uint8List.fromList(_buf.sublist(dataStart, dataStart + len));
    final crcRx = (_buf[dataStart + len] << 8) | _buf[dataStart + len + 1];
    if (crc16(payload) != crcRx) return (-1, null);
    return (total, payload);
  }

  void reset() => _buf.clear();
}
