/// Big-endian byte reader/writer mirroring VESC Tool `VByteArray` (vbAppend* /
/// vbPopFront*). VESC payloads are big-endian — do NOT reuse the app's own
/// little-endian NotifBridge helpers here.
library;

import 'dart:convert';
import 'dart:math' as math;
import 'dart:typed_data';

/// Builds a VESC request payload (big-endian).
class VbWriter {
  final BytesBuilder _b = BytesBuilder();

  void u8(int v) => _b.addByte(v & 0xFF);
  void i8(int v) => _b.addByte(v & 0xFF);

  void u16(int v) {
    _b.addByte((v >> 8) & 0xFF);
    _b.addByte(v & 0xFF);
  }

  void i16(int v) => u16(v);

  void u32(int v) {
    _b.addByte((v >> 24) & 0xFF);
    _b.addByte((v >> 16) & 0xFF);
    _b.addByte((v >> 8) & 0xFF);
    _b.addByte(v & 0xFF);
  }

  void i32(int v) => u32(v);

  void bytes(List<int> v) => _b.add(v);

  /// NUL-terminated string (vbAppendString).
  void cstr(String s) {
    _b.add(utf8.encode(s));
    _b.addByte(0);
  }

  int get length => _b.length;
  Uint8List take() => _b.toBytes();
}

/// Pops fields off the front of a VESC reply payload (big-endian).
class VbReader {
  final Uint8List _d;
  int _i = 0;
  VbReader(this._d);

  int get remaining => _d.length - _i;

  int u8() => remaining < 1 ? 0 : _d[_i++];

  int i8() {
    final v = u8();
    return v >= 0x80 ? v - 0x100 : v;
  }

  int u16() {
    if (remaining < 2) {
      _i = _d.length;
      return 0;
    }
    final v = (_d[_i] << 8) | _d[_i + 1];
    _i += 2;
    return v;
  }

  int i16() {
    final v = u16();
    return v >= 0x8000 ? v - 0x10000 : v;
  }

  int u32() {
    if (remaining < 4) {
      _i = _d.length;
      return 0;
    }
    final v = (_d[_i] << 24) |
        (_d[_i + 1] << 16) |
        (_d[_i + 2] << 8) |
        _d[_i + 3];
    _i += 4;
    return v & 0xFFFFFFFF;
  }

  int i32() {
    final v = u32();
    return v >= 0x80000000 ? v - 0x100000000 : v;
  }

  /// int16 / scale (vbPopFrontDouble16).
  double d16(double scale) => i16() / scale;

  /// int32 / scale (vbPopFrontDouble32).
  double d32(double scale) => i32() / scale;

  /// 4-byte "auto" compressed float (vbPopFrontDouble32Auto).
  double d32Auto() {
    final res = u32();
    var e = (res >> 23) & 0xFF;
    final fr = res & 0x7FFFFF;
    final negative = (res & 0x80000000) != 0;
    var f = 0.0;
    if (e != 0 || fr != 0) {
      f = fr / (8388608.0 * 2.0) + 0.5;
      e -= 126;
    }
    if (negative) f = -f;
    return f * math.pow(2.0, e).toDouble();
  }

  /// NUL-terminated string (vbPopFrontString).
  String cstr() {
    final start = _i;
    while (_i < _d.length && _d[_i] != 0) {
      _i++;
    }
    final s = utf8.decode(_d.sublist(start, _i), allowMalformed: true);
    if (_i < _d.length) _i++; // skip the terminator
    return s;
  }

  /// Remaining bytes (consumes them).
  Uint8List rest() {
    final r = _d.sublist(_i);
    _i = _d.length;
    return r;
  }
}
