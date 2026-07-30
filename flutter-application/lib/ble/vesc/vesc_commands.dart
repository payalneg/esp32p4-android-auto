/// LISP COMM command builders + reply parsers, ported from VESC Tool
/// `commands.cpp` / `codeloader.cpp`. Payloads passed to the parsers INCLUDE
/// the leading command byte (that's how they arrive from the packet decoder);
/// the builders produce the payload the packet framer then wraps.
library;

import 'dart:convert';
import 'dart:typed_data';

import '../lisp_models.dart';
import 'vesc_buffer.dart';

class VescCmd {
  /// Plain `commands_printf()` output. Some LispBM builds route `(print ...)`
  /// through this rather than [lispPrint], so both are treated as console text.
  static const commPrint = 21;

  static const lispReadCode = 130;
  static const lispWriteCode = 131;
  static const lispEraseCode = 132;
  static const lispSetRunning = 133;
  static const lispGetStats = 134;

  /// `commands_printf_lisp()` — the script's own `(print ...)` output, pushed
  /// asynchronously (never in reply to a request).
  static const lispPrint = 135;
}

// ---- request builders ----

Uint8List buildLispRead(int len, int offset) =>
    (VbWriter()..u8(VescCmd.lispReadCode)..i32(len)..i32(offset)).take();

Uint8List buildLispWrite(int offset, Uint8List chunk) =>
    (VbWriter()..u8(VescCmd.lispWriteCode)..u32(offset)..bytes(chunk)).take();

Uint8List buildLispErase(int size) =>
    (VbWriter()..u8(VescCmd.lispEraseCode)..i32(size)).take();

Uint8List buildLispSetRunning(bool run) =>
    (VbWriter()..u8(VescCmd.lispSetRunning)..i8(run ? 1 : 0)).take();

Uint8List buildLispGetStats(bool all) =>
    (VbWriter()..u8(VescCmd.lispGetStats)..i8(all ? 1 : 0)).take();

// ---- reply parsers (payload includes the command byte at [0]) ----

({int total, int offset, Uint8List data}) parseLispRead(Uint8List p) {
  final r = VbReader(p)..u8(); // drop command byte
  final total = r.i32();
  final offset = r.i32();
  return (total: total, offset: offset, data: r.rest());
}

({bool ok, int offset}) parseLispWrite(Uint8List p) {
  final r = VbReader(p)..u8();
  final ok = r.i8() != 0;
  final offset = r.u32();
  return (ok: ok, offset: offset);
}

bool parseLispEraseOk(Uint8List p) => p.length > 1 && p[1] != 0;

bool parseLispRunOk(Uint8List p) => p.length > 1 && p[1] != 0;

/// Decode a [VescCmd.lispPrint] / [VescCmd.commPrint] payload into console
/// lines.
///
/// The VESC writes the text at `payload[1..]` using `strlen` as the length:
/// there is NO guaranteed NUL terminator, the text may or may not end with a
/// newline, and one packet can carry several lines. Nothing here may throw — a
/// malformed byte must not take the console down — hence
/// `allowMalformed: true`.
List<String> parseVescPrint(Uint8List p, {int maxLineChars = 512}) {
  if (p.length < 2) return const [];
  var end = p.length;
  while (end > 1) {
    final b = p[end - 1];
    if (b != 0 && b != 0x0a && b != 0x0d) break;
    end--;
  }
  if (end <= 1) return const [];
  final text = utf8.decode(p.sublist(1, end), allowMalformed: true);
  final out = <String>[];
  for (final raw in text.split('\n')) {
    final line = raw.replaceAll('\r', '').trimRight();
    if (line.isEmpty) continue;
    out.add(line.length > maxLineChars ? line.substring(0, maxLineChars) : line);
  }
  return out;
}

LispStats parseLispStats(Uint8List p) {
  final r = VbReader(p)..u8();
  final cpu = r.d16(1e2);
  final heap = r.d16(1e2);
  final mem = r.d16(1e2);
  final stack = r.d16(1e2);
  final doneCtx = r.cstr();
  final bindings = <LispBinding>[];
  while (r.remaining > 0) {
    final name = r.cstr();
    if (r.remaining < 4) break; // no value follows — malformed tail
    bindings.add(LispBinding(name, r.d32Auto()));
  }
  return LispStats(
    cpu: cpu,
    heap: heap,
    mem: mem,
    stack: stack,
    doneCtx: doneCtx,
    bindings: bindings,
  );
}
