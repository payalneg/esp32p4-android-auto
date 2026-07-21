/// CRC16-CCITT (XMODEM, poly 0x1021, init 0) — the checksum used by the VESC
/// packet framing. Ported from VESC Tool `packet.cpp` (Packet::crc16). The
/// 256-entry table is generated at load rather than transcribed to avoid
/// copy errors; it produces the exact same values as the hard-coded table.
library;

import 'dart:typed_data';

final Uint16List _crcTab = _buildTable();

Uint16List _buildTable() {
  final t = Uint16List(256);
  for (var i = 0; i < 256; i++) {
    var crc = i << 8;
    for (var j = 0; j < 8; j++) {
      crc = (crc & 0x8000) != 0 ? ((crc << 1) ^ 0x1021) : (crc << 1);
      crc &= 0xFFFF;
    }
    t[i] = crc;
  }
  return t;
}

/// CRC16 over [buf] (or its first [len] bytes). Matches
/// `crc16_tab[(((cksum >> 8) ^ *buf++) & 0xFF)] ^ (cksum << 8)`.
int crc16(Uint8List buf, [int? len]) {
  final n = len ?? buf.length;
  var cksum = 0;
  for (var i = 0; i < n; i++) {
    cksum = _crcTab[((cksum >> 8) ^ buf[i]) & 0xFF] ^ ((cksum << 8) & 0xFFFF);
    cksum &= 0xFFFF;
  }
  return cksum & 0xFFFF;
}
