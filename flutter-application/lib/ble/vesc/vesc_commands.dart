/// LISP COMM command builders + reply parsers, ported from VESC Tool
/// `commands.cpp` / `codeloader.cpp`. Payloads passed to the parsers INCLUDE
/// the leading command byte (that's how they arrive from the packet decoder);
/// the builders produce the payload the packet framer then wraps.
library;

import 'dart:typed_data';

import '../lisp_models.dart';
import 'vesc_buffer.dart';

class VescCmd {
  static const lispReadCode = 130;
  static const lispWriteCode = 131;
  static const lispEraseCode = 132;
  static const lispSetRunning = 133;
  static const lispGetStats = 134;
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
