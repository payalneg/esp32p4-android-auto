/// Speaks the VESC protocol over the head unit's NUS bridge (the same path
/// VESC Tool uses): writes framed packets to NUS RX, reassembles replies off
/// NUS TX, and matches replies to requests by command id. All ops are
/// serialized (one VESC transaction in flight at a time) to stay clear of the
/// firmware's single CAN reassembly buffer.
///
/// Errors are thrown as [VescLispException] carrying an i18n key; the UI
/// localizes via `t()`.
library;

import 'dart:async';
import 'dart:typed_data';

import 'package:flutter/services.dart' show PlatformException;
import 'package:flutter_blue_plus/flutter_blue_plus.dart';

import '../ble_service.dart';
import '../lisp_models.dart';
import 'vesc_code_loader.dart';
import 'vesc_commands.dart';
import 'vesc_packet.dart';

class VescLispException implements Exception {
  final String key;
  const VescLispException(this.key);
  @override
  String toString() => 'VescLispException($key)';
}

class VescLink {
  VescLink._();
  static final VescLink instance = VescLink._();

  static const _chunk = 384; // VESC Tool upload chunk size

  final _decoder = VescPacketDecoder();
  StreamSubscription<List<int>>? _sub;
  BluetoothCharacteristic? _boundTx;

  // One outstanding request per command id (safe because ops are serialized).
  final _pending = <int, Completer<Uint8List>>{};

  Future<void> _opLock = Future.value();

  bool get supported => BleService.instance.supportsLisp;

  /// Serialize whole operations so two transactions can't interleave on the
  /// shared link / CAN reassembler.
  Future<T> _serial<T>(Future<T> Function() op) async {
    final prev = _opLock;
    final done = Completer<void>();
    _opLock = done.future;
    try {
      await prev;
    } catch (_) {}
    try {
      return await op();
    } finally {
      done.complete();
    }
  }

  Future<void> _ensureSubscribed() async {
    final tx = BleService.instance.nusTxChar;
    final rx = BleService.instance.nusRxChar;
    if (tx == null || rx == null || !BleService.instance.isConnected) {
      throw const VescLispException('lisp.err.noconn');
    }
    if (identical(tx, _boundTx) && _sub != null) return;
    await _sub?.cancel();
    _decoder.reset();
    await tx.setNotifyValue(true);
    _boundTx = tx;
    _sub = tx.onValueReceived.listen((raw) {
      final payloads = _decoder.feed(Uint8List.fromList(raw));
      for (final p in payloads) {
        if (p.isEmpty) continue;
        final c = _pending.remove(p[0]);
        if (c != null && !c.isCompleted) c.complete(p);
        // Unsolicited packets (the head unit forwards its own poller replies
        // and LISP print output) simply don't match a pending request.
      }
    });
  }

  Future<void> _writeRx(Uint8List packet) async {
    final rx = BleService.instance.nusRxChar;
    if (rx == null) throw const VescLispException('lisp.err.noconn');
    final chunkSize = (BleService.instance.negotiatedMtu - 3).clamp(20, 244);
    var off = 0;
    while (off < packet.length) {
      final end =
          (off + chunkSize) < packet.length ? off + chunkSize : packet.length;
      await _writeChunk(rx, Uint8List.sublistView(packet, off, end));
      off = end;
    }
  }

  Future<void> _writeChunk(BluetoothCharacteristic ch, Uint8List chunk) async {
    var attempt = 0;
    while (true) {
      try {
        await ch.write(chunk, withoutResponse: attempt < 2);
        return;
      } on PlatformException catch (e) {
        final msg = (e.message ?? '').toLowerCase();
        final busy = msg.contains('busy') || msg.contains('201');
        if (!busy || attempt >= 8) rethrow;
        attempt++;
        await Future.delayed(Duration(milliseconds: 15 * attempt));
      }
    }
  }

  /// Send [reqPayload] and await the reply whose command id is [replyCmd].
  /// Retries the whole send/wait up to [tries] times on timeout.
  Future<Uint8List> _sendAndWait(Uint8List reqPayload, int replyCmd,
      {required Duration timeout, int tries = 1}) async {
    final framed = VescPacket.encode(reqPayload);
    for (var attempt = 0; attempt < tries; attempt++) {
      final c = Completer<Uint8List>();
      _pending[replyCmd] = c;
      try {
        await _writeRx(framed);
        return await c.future.timeout(timeout);
      } on TimeoutException {
        _pending.remove(replyCmd);
      } catch (_) {
        _pending.remove(replyCmd);
        rethrow;
      }
    }
    throw const VescLispException('lisp.err.timeout');
  }

  // ---- public operations ----

  /// Read the LISP script currently stored on the VESC and return its text.
  Future<String> readCode({void Function(double)? onProgress}) =>
      _serial(() async {
        await _ensureSubscribed();
        var total = -1;
        final buf = BytesBuilder();
        var resync = 0;
        while (true) {
          final expected = buf.length;
          if (total >= 0 && expected >= total) break;
          final want = total < 0
              ? 10
              : ((total - expected) > 400 ? 400 : (total - expected));
          final r = parseLispRead(await _sendAndWait(
              buildLispRead(want <= 0 ? 10 : want, expected),
              VescCmd.lispReadCode,
              timeout: const Duration(milliseconds: 1500),
              tries: 5));
          if (total < 0) total = r.total;
          if (total <= 0) return '';
          if (r.offset != expected) {
            if (++resync > 10) throw const VescLispException('lisp.err.read');
            continue; // stale/misaligned reply — re-request this offset
          }
          if (r.data.isEmpty) throw const VescLispException('lisp.err.read');
          buf.add(r.data);
          onProgress?.call(buf.length / total);
        }
        final data = buf.toBytes();
        final trimmed = data.length > total
            ? Uint8List.sublistView(data, 0, total)
            : data;
        return unpackLispCode(trimmed);
      });

  /// Erase + write [code] to the VESC, optionally starting it afterwards.
  Future<void> uploadCode(String code,
          {bool run = false, void Function(double)? onProgress}) =>
      _serial(() async {
        await _ensureSubscribed();
        final packed = packLispCode(code);
        final blob = buildUploadBlob(packed);

        // Erase (packed size + slack), like VESC Tool's on_uploadButton.
        final eraseReply = await _sendAndWait(
            buildLispErase(packed.length + 100), VescCmd.lispEraseCode,
            timeout: const Duration(seconds: 8), tries: 1);
        if (!parseLispEraseOk(eraseReply)) {
          throw const VescLispException('lisp.err.erase');
        }

        // Chunked write of the [len][crc][packed] blob.
        var offset = 0;
        onProgress?.call(0);
        while (offset < blob.length) {
          final end =
              (offset + _chunk) < blob.length ? offset + _chunk : blob.length;
          final wr = parseLispWrite(await _sendAndWait(
              buildLispWrite(offset, Uint8List.sublistView(blob, offset, end)),
              VescCmd.lispWriteCode,
              timeout: const Duration(seconds: 1),
              tries: 5));
          if (!wr.ok) throw const VescLispException('lisp.err.write');
          offset = end;
          onProgress?.call(offset / blob.length);
        }

        if (run) await _setRunningBestEffort(true);
      });

  /// Start (true) / stop (false) the stored script.
  Future<void> setRunning(bool run) => _serial(() async {
        await _ensureSubscribed();
        await _setRunningBestEffort(run);
      });

  /// SET_RUNNING with a short wait; a missing ack is not treated as failure
  /// (VESC Tool's run/stop buttons are fire-and-forget too).
  Future<void> _setRunningBestEffort(bool run) async {
    try {
      await _sendAndWait(buildLispSetRunning(run), VescCmd.lispSetRunning,
          timeout: const Duration(milliseconds: 1500), tries: 1);
    } on VescLispException {
      // command was sent; the ack may just not have come back
    }
  }

  /// Fetch one LISP runtime-stats snapshot (cpu/heap/mem/stack + globals).
  Future<LispStats> getStats({bool all = true}) => _serial(() async {
        await _ensureSubscribed();
        final reply = await _sendAndWait(
            buildLispGetStats(all), VescCmd.lispGetStats,
            timeout: const Duration(milliseconds: 1000), tries: 2);
        return parseLispStats(reply);
      });
}
