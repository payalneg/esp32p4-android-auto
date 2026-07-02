/// LISP code (un)packing, ported from VESC Tool `codeloader.cpp`
/// (lispPackImports / lispUpload / lispUnpackImports) for the common
/// plain-script (no `(import ...)`) case.
///
/// Packed layout for a plain script:
///   [u16 flags=0][code bytes][NUL][i16 num_imports=0]
/// Upload blob wraps that:
///   [u32 (packed.length - 2)][u16 crc16(packed)][packed]
/// The `- 2` (dropping the flags field from the length) matches VESC Tool
/// exactly — the VESC firmware relies on it.
library;

import 'dart:convert';
import 'dart:typed_data';

import 'crc16.dart';
import 'vesc_buffer.dart';

/// Pack a plain LISP script (no imports) like `lispPackImports`.
Uint8List packLispCode(String code) {
  final w = VbWriter();
  w.u16(0); // flags
  final codeBytes = utf8.encode(code);
  w.bytes(codeBytes);
  if (codeBytes.isEmpty || codeBytes.last != 0) w.u8(0); // NUL-terminate
  w.i16(0); // num_imports = 0
  return w.take();
}

/// Wrap packed data for upload: `[u32 len][u16 crc16][packed]`.
Uint8List buildUploadBlob(Uint8List packed) {
  final w = VbWriter();
  w.u32(packed.length - 2);
  w.u16(crc16(packed));
  w.bytes(packed);
  return w.take();
}

/// Recover the code text from a read-back blob (inverse of [packLispCode] for
/// the no-import case). Strips the leading flags and returns everything up to
/// the first NUL.
String unpackLispCode(Uint8List data) {
  var d = data;
  if (d.length > 2 && d[0] == 0 && d[1] == 0) d = Uint8List.sublistView(d, 2);
  final end = d.indexOf(0);
  final codeBytes = end >= 0 ? Uint8List.sublistView(d, 0, end) : d;
  return utf8.decode(codeBytes, allowMalformed: true);
}
