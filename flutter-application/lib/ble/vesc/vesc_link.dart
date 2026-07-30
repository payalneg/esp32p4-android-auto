/// Speaks the VESC protocol over a Nordic UART Service link (the same path
/// VESC Tool uses): writes framed packets to NUS RX, reassembles replies off
/// NUS TX, and matches replies to requests by command id. All ops are
/// serialized (one VESC transaction in flight at a time) to stay clear of the
/// head unit's single CAN reassembly buffer.
///
/// Which link that is — the head unit's bridge or a stand-alone VESC BLE
/// adapter — is decided by [VescTarget]; this layer only sees a
/// [NusTransport]. The bytes are identical either way.
///
/// Errors are thrown as [VescLispException] carrying an i18n key; the UI
/// localizes via `t()`.
library;

import 'dart:async';
import 'dart:typed_data';

import 'package:flutter/foundation.dart' show debugPrint;

import '../lisp_models.dart';
import 'lisp_console_ring.dart';
import 'nus_transport.dart';
import 'vesc_code_loader.dart';
import 'vesc_commands.dart';
import 'vesc_errors.dart';
import 'vesc_packet.dart';
import 'vesc_target.dart';

export 'vesc_errors.dart' show VescLispException;

class VescLink {
  VescLink._();
  static final VescLink instance = VescLink._();

  static const _chunk = 384; // VESC Tool upload chunk size

  /// Payload bytes asked for per READ_CODE request.
  static const _readChunk = 400;

  /// READ_CODE requests allowed in flight at once.
  ///
  /// A read is latency-bound, not bandwidth-bound: one 400 B round-trip costs
  /// a BLE connection interval each way plus — on the head-unit path — the
  /// CAN hop behind it, and a strictly serial loop leaves both idle most of
  /// the time. Replies carry their own offset, so several can be outstanding
  /// and matched on arrival. The VESC serialises them anyway (its CAN/UART
  /// handler processes one packet at a time), which is the point: the next
  /// request is already queued there when it finishes the previous one.
  static const _readWindow = 3;

  final _decoder = VescPacketDecoder();
  StreamSubscription<List<int>>? _sub;
  NusTransport? _transport;
  Object? _boundKey;

  // One outstanding request per command id (safe because ops are serialized).
  final _pending = <int, Completer<Uint8List>>{};

  /// In-flight pipelined READ_CODE requests, keyed by the offset they asked
  /// for — the reply repeats it, which is what makes the pipelining possible.
  /// Only populated while [readCode] runs; [_pending] handles every other
  /// command (and READ_CODE too, whenever the pipeline is idle).
  final _readWaiters = <int, Completer<({int total, Uint8List data})>>{};
  bool _readPipelined = false;

  /// Serialises the GATT writes themselves (not the waits) so two pipelined
  /// requests can't interleave on the characteristic and trip the
  /// write-request-busy path in [writeChunked].
  Future<void> _writeLock = Future.value();

  /// Asynchronous script output (`(print ...)`), which arrives unsolicited and
  /// matches no pending request.
  final _console = LispConsoleRing();

  /// How many payloads of each command id arrived without matching a request.
  /// On the head-unit path this is dominated by ids 4 and 134 — the P4's own
  /// 10 Hz RT-data and LISP-stats polls, forwarded to us verbatim by the
  /// bridge. Useful for working out the framing of an unrecognised id.
  final _unmatched = <int, int>{};

  /// Dump unrecognised payloads into the console as hex. Off by default: the
  /// head unit's polling would otherwise flood the ring.
  bool consoleDebug = false;

  Future<void> _opLock = Future.value();

  // Per-transfer counters, reset by [_beginXfer].
  int _retries = 0;
  int _resyncs = 0;
  int _chunkCount = 0;
  int _maxChunkMs = 0;
  final _xferClock = Stopwatch();

  bool get supported => VescTarget.instance.available;

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

  /// Run a long transfer at high BLE connection priority (Android drops the
  /// connection interval from ~45 ms to 7.5-15 ms), restoring balanced after.
  /// Dozens of serialized round-trips otherwise spend most of their time
  /// waiting out the interval.
  Future<T> _fastLink<T>(NusTransport t, Future<T> Function() op) async {
    await t.setFast(true);
    try {
      return await op();
    } finally {
      await t.setFast(false);
    }
  }

  /// Resolve the active transport, (re)binding the notification subscription
  /// when the underlying link changed (reconnect, or a different adapter).
  Future<NusTransport> _bind() async {
    final t = await VescTarget.instance.require();
    final key = t.bindKey;
    if (identical(_transport, t) && identical(_boundKey, key) && _sub != null) {
      return t;
    }
    final rebind = _sub != null;
    await _sub?.cancel();
    _decoder.reset();
    _transport = t;
    _boundKey = key;
    // The ring deliberately survives a rebind — a reader must be able to tell
    // "the script went quiet" from "we lost the link and reconnected".
    if (rebind) _console.add('— link rebound —', kind: 'marker');
    _sub = t.inbound.listen((raw) {
      final payloads = _decoder.feed(Uint8List.fromList(raw));
      for (final p in payloads) {
        if (p.isEmpty) continue;
        final id = p[0];
        // Script output is pushed asynchronously and never answers a request.
        if (id == VescCmd.lispPrint || id == VescCmd.commPrint) {
          for (final line in parseVescPrint(p)) {
            _console.add(line);
          }
          continue;
        }
        // Pipelined read in progress: route READ_CODE replies by their offset
        // instead of by command id (several are outstanding at once).
        if (_readPipelined && id == VescCmd.lispReadCode) {
          if (_completeReadReply(p)) continue;
          // No waiter for that offset — a late reply to a request we already
          // timed out and re-sent, or a stale one from a previous transfer.
          _resyncs++;
          _unmatched[id] = (_unmatched[id] ?? 0) + 1;
          continue;
        }
        final c = _pending.remove(id);
        if (c != null && !c.isCompleted) {
          c.complete(p);
          continue;
        }
        // Unsolicited: the head unit forwards its own poller replies too.
        _unmatched[id] = (_unmatched[id] ?? 0) + 1;
        if (consoleDebug) _console.addRaw(id, p);
      }
    });
    return t;
  }

  /// Live asynchronous output from the running script.
  Stream<LispConsoleLine> get prints => _console.stream;

  /// The buffered console, for readers that poll rather than subscribe.
  LispConsoleRing get console => _console;

  Map<int, int> get unmatchedHistogram => Map.unmodifiable(_unmatched);

  /// Send [reqPayload] and await the reply whose command id is [replyCmd].
  /// Retries the whole send/wait up to [tries] times on timeout.
  Future<Uint8List> _sendAndWait(Uint8List reqPayload, int replyCmd,
      {required Duration timeout, int tries = 1}) async {
    final framed = VescPacket.encode(reqPayload);
    final t = _transport;
    if (t == null) throw const VescLispException('lisp.err.noconn');
    for (var attempt = 0; attempt < tries; attempt++) {
      final c = Completer<Uint8List>();
      _pending[replyCmd] = c;
      try {
        await _write(t, framed);
        return await c.future.timeout(timeout);
      } on TimeoutException {
        _pending.remove(replyCmd);
        _retries++;
      } catch (_) {
        _pending.remove(replyCmd);
        rethrow;
      }
    }
    throw const VescLispException('lisp.err.timeout');
  }

  /// One packet onto the wire at a time — see [_writeLock].
  Future<void> _write(NusTransport t, Uint8List framed) {
    final done = Completer<void>();
    final prev = _writeLock;
    _writeLock = done.future;
    return prev.catchError((_) {}).then((_) => t.write(framed)).whenComplete(
        () => done.complete());
  }

  /// Hand a READ_CODE payload to the waiter that asked for its offset.
  /// Returns false when nothing was waiting on it.
  bool _completeReadReply(Uint8List p) {
    final ({int total, int offset, Uint8List data}) r;
    try {
      r = parseLispRead(p);
    } catch (_) {
      return false; // truncated/garbled — let the requester time out and retry
    }
    final c = _readWaiters.remove(r.offset);
    if (c == null || c.isCompleted) return false;
    c.complete((total: r.total, data: r.data));
    return true;
  }

  /// One pipelined READ_CODE round-trip. Unlike [_sendAndWait] the reply is
  /// matched on [offset], so several of these may be awaited concurrently.
  Future<({int total, Uint8List data})> _readAt(int offset, int len,
      {required Duration timeout, int tries = 5}) async {
    final t = _transport;
    if (t == null) throw const VescLispException('lisp.err.noconn');
    final framed = VescPacket.encode(buildLispRead(len, offset));
    for (var attempt = 0; attempt < tries; attempt++) {
      final c = Completer<({int total, Uint8List data})>();
      _readWaiters[offset] = c;
      final clock = Stopwatch()..start();
      try {
        await _write(t, framed);
        final r = await c.future.timeout(timeout);
        clock.stop();
        _countChunk(clock.elapsedMilliseconds);
        return r;
      } on TimeoutException {
        _readWaiters.remove(offset);
        _retries++;
      } catch (_) {
        _readWaiters.remove(offset);
        rethrow;
      }
    }
    throw const VescLispException('lisp.err.timeout');
  }

  // ---- transfer instrumentation ----
  //
  // Read/upload speed over BLE is dominated by per-chunk round-trip latency
  // (and, on the head-unit path, by the CAN hop behind it). These counters let
  // a real transfer be measured on hardware — logged to logcat and handed to
  // the UI — instead of guessed at.

  void _beginXfer() {
    _retries = 0;
    _resyncs = 0;
    _chunkCount = 0;
    _maxChunkMs = 0;
    _xferClock
      ..reset()
      ..start();
  }

  void _countChunk(int ms) {
    _chunkCount++;
    if (ms > _maxChunkMs) _maxChunkMs = ms;
  }

  VescXferStats _endXfer(String what, int bytes) {
    _xferClock.stop();
    final s = VescXferStats(
      bytes: bytes,
      ms: _xferClock.elapsedMilliseconds,
      chunks: _chunkCount,
      retries: _retries,
      resyncs: _resyncs,
      maxChunkMs: _maxChunkMs,
      link: _transport?.label ?? '-',
      mtu: _transport?.mtu ?? 0,
    );
    debugPrint('[vesc] $what ${s.bytes} B / ${(s.ms / 1000).toStringAsFixed(1)} s'
        ' = ${s.kbs.toStringAsFixed(1)} KB/s, ${s.chunks} chunks,'
        ' avg ${s.avgChunkMs} ms, max ${s.maxChunkMs} ms,'
        ' retries ${s.retries}, resync ${s.resyncs},'
        ' link ${s.link}, mtu ${s.mtu}');
    return s;
  }

  /// Stats of the last completed read/upload (also returned by those calls).
  VescXferStats? lastStats;

  // ---- public operations ----

  /// Read the LISP script currently stored on the VESC and return its text.
  ///
  /// Runs [_readWindow] requests in flight: the script is fetched into a
  /// preallocated buffer by offset rather than appended in arrival order, so
  /// replies may (and do) come back out of order.
  Future<({String code, VescXferStats stats})> readCode(
          {void Function(double)? onProgress}) =>
      _serial(() async {
        final t = await _bind();
        return _fastLink(t, () async {
          _beginXfer();
          _readWaiters.clear();
          _readPipelined = true;
          try {
            const rtt = Duration(milliseconds: 1500);

            // Size probe. Deliberately still the 10-byte request VESC Tool
            // opens with — the firmware clamps over-long reads, and this is
            // the one request whose length we can't derive from `total` yet.
            final head = await _readAt(0, 10, timeout: rtt);
            final total = head.total;
            if (total <= 0) {
              lastStats = _endXfer('read', 0);
              return (code: '', stats: lastStats!);
            }

            final out = Uint8List(total);
            var received = 0;
            void store(int offset, Uint8List data) {
              var n = data.length;
              if (n > total - offset) n = total - offset;
              if (n <= 0) return;
              out.setRange(offset, offset + n, data);
              received += n;
              onProgress?.call(received / total);
            }

            store(0, head.data);
            var next = head.data.length > total ? total : head.data.length;
            Object? failure;

            // One worker owns one slot at a time. A failing worker records the
            // error and lets the others wind down at their next slot rather
            // than leaving requests in flight that would still be writing into
            // `out` after readCode has thrown.
            Future<void> worker() async {
              try {
                while (failure == null) {
                  // Claim a slot. No await between reading and updating
                  // `next`, so two workers can't take the same one.
                  final slotStart = next;
                  if (slotStart >= total) return;
                  final slotEnd = (total - slotStart) > _readChunk
                      ? slotStart + _readChunk
                      : total;
                  next = slotEnd;

                  var o = slotStart;
                  while (o < slotEnd) {
                    final want = slotEnd - o;
                    final r = await _readAt(o, want, timeout: rtt);
                    if (r.data.isEmpty) {
                      throw const VescLispException('lisp.err.read');
                    }
                    final take = r.data.length > want ? want : r.data.length;
                    store(o, Uint8List.sublistView(r.data, 0, take));
                    o += take; // short reply: come back for this slot's tail
                  }
                }
              } catch (e) {
                failure ??= e;
              }
            }

            await Future.wait([for (var i = 0; i < _readWindow; i++) worker()]);
            if (failure != null) throw failure!;

            lastStats = _endXfer('read', total);
            return (code: unpackLispCode(out), stats: lastStats!);
          } finally {
            _readPipelined = false;
            _readWaiters.clear();
          }
        });
      });

  /// Erase + write [code] to the VESC, optionally starting it afterwards.
  ///
  /// [stopFirst] halts the running script before erasing. The agent always
  /// asks for it — erasing the flash out from under a live script on a motor
  /// controller is not something to do hopefully — while the editor's own
  /// Upload button keeps the historical behaviour.
  Future<VescXferStats> uploadCode(String code,
          {bool run = false,
          bool stopFirst = false,
          void Function(double)? onProgress}) =>
      _serial(() async {
        final t = await _bind();
        return _fastLink(t, () async {
          _beginXfer();
          final packed = packLispCode(code);
          final blob = buildUploadBlob(packed);

          if (stopFirst) await _setRunningBestEffort(false);

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
            final chunkClock = Stopwatch()..start();
            final wr = parseLispWrite(await _sendAndWait(
                buildLispWrite(offset, Uint8List.sublistView(blob, offset, end)),
                VescCmd.lispWriteCode,
                timeout: const Duration(seconds: 1),
                tries: 5));
            chunkClock.stop();
            _countChunk(chunkClock.elapsedMilliseconds);
            if (!wr.ok) throw const VescLispException('lisp.err.write');
            offset = end;
            onProgress?.call(offset / blob.length);
          }

          if (run) await _setRunningBestEffort(true);
          lastStats = _endXfer('upload', blob.length);
          return lastStats!;
        });
      });

  /// Start (true) / stop (false) the stored script.
  Future<void> setRunning(bool run) => _serial(() async {
        await _bind();
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
        await _bind();
        final reply = await _sendAndWait(
            buildLispGetStats(all), VescCmd.lispGetStats,
            timeout: const Duration(milliseconds: 1000), tries: 2);
        return parseLispStats(reply);
      });
}
