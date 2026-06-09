/// High-level device file manager over BLE. Mirrors firmware main/ble_files.c.
///
/// Sits on top of [BleService]'s file-manager characteristics. One operation
/// in flight at a time (serialized internally). All ops throw
/// [FileOpException] (with an i18n key) on a device-reported error; the UI
/// localizes via `t()`.
library;

import 'dart:async';
import 'dart:typed_data';

import 'package:flutter/foundation.dart';
import 'package:crypto/crypto.dart';
import 'package:flutter/services.dart' show PlatformException;
import 'package:flutter_blue_plus/flutter_blue_plus.dart';

import 'ble_service.dart';
import 'file_ops.dart';
import 'protocol.dart';

class FileManager {
  FileManager._();
  static final FileManager instance = FileManager._();

  final _decoder = ChunkedDecoder();
  final _status = StreamController<({int status, int detail})>.broadcast();
  final _bulk = StreamController<Uint8List>.broadcast();
  final _bulkProgress = StreamController<({int recv, int total})>.broadcast();

  StreamSubscription<List<int>>? _sub;
  BluetoothCharacteristic? _boundCtrl;

  Future<void> _opLock = Future.value();

  bool get supported => BleService.instance.supportsFileManager;

  /// Serialize ops so two transfers can't interleave on the shared channel.
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

  /// Bind the CTRL notify stream once and demux status frames vs. bulk chunks.
  Future<void> _ensureSubscribed() async {
    final ctrl = BleService.instance.fileCtrlChar;
    if (ctrl == null || !BleService.instance.isConnected) {
      throw const FileOpException('files.err.noconn');
    }
    if (identical(ctrl, _boundCtrl) && _sub != null) return;
    await _sub?.cancel();
    await ctrl.setNotifyValue(true);
    _boundCtrl = ctrl;
    _sub = ctrl.onValueReceived.listen((raw) {
      final u = Uint8List.fromList(raw);
      // A bulk chunk: 8-byte chunk header whose first byte is PDU_TYPE_FILE.
      if (u.length >= kChunkHeaderLen && u[0] == PduType.file.code) {
        final bd = ByteData.sublistView(u);
        final flags = bd.getUint8(1);
        final total = bd.getUint32(4, Endian.little);
        if ((flags & ChunkFlags.start) != 0) _bulkRecv = 0;
        _bulkTotal = total;
        _bulkRecv += (u.length - kChunkHeaderLen);
        _bulkProgress.add((recv: _bulkRecv, total: _bulkTotal));
        final out = _decoder.feed(u);
        debugPrint('[files] chunk flags=$flags len=${u.length} recv=$_bulkRecv/$_bulkTotal'
            '${out != null ? " -> body ${out.body.length}B sub=${out.body.isNotEmpty ? out.body[0] : -1}" : ""}');
        if (out != null && out.type == PduType.file) _bulk.add(out.body);
        return;
      }
      // Otherwise a 5-byte status frame [u8 status][u32 detail].
      if (u.isNotEmpty) {
        final bd = ByteData.sublistView(u);
        final status = bd.getUint8(0);
        final detail = u.length >= 5 ? bd.getUint32(1, Endian.little) : 0;
        debugPrint('[files] status=0x${status.toRadixString(16)} detail=$detail len=${u.length}');
        _status.add((status: status, detail: detail));
      }
    });
  }

  int _bulkRecv = 0;
  int _bulkTotal = 0;

  Never _raise(int detail, {bool notReady = false}) =>
      throw FileOpException(FileOps.errKey(detail), notReady: notReady);

  Uint8List _ctrlReq(int op, [Uint8List? args]) {
    final b = BytesBuilder();
    b.addByte(op);
    if (args != null) b.add(args);
    return b.takeBytes();
  }

  Future<void> _writeCtrl(Uint8List bytes) async {
    final ctrl = BleService.instance.fileCtrlChar;
    if (ctrl == null) throw const FileOpException('files.err.noconn');
    await ctrl.write(bytes, withoutResponse: false);
  }

  // ---- operations ----

  /// List [path] ("/" → the synthetic drive list). Throws [FileOpException]
  /// (notReady=true while /vescfs is still mounting/formatting).
  Future<RemoteListing> listDir(String path) => _serial(() async {
        debugPrint('[files] listDir("$path") start');
        await _ensureSubscribed();
        Uint8List? body;
        final bulkSub = _bulk.stream.listen((b) => body = b);
        // Generous: a big folder over a slow link (≈48 ms interval) can be
        // hundreds of chunks and take many seconds to stream in.
        final terminal = _status.stream
            .firstWhere((e) =>
                e.status == FileOps.stDone ||
                e.status == FileOps.stError ||
                e.status == FileOps.stNotReady)
            .timeout(const Duration(seconds: 90));
        try {
          await _writeCtrl(_ctrlReq(FileOps.opList, Tlv.encode({1: path})));
          final r = await terminal;
          if (r.status == FileOps.stNotReady) _raise(FileOps.errNotReady, notReady: true);
          if (r.status == FileOps.stError) _raise(r.detail);
          return RemoteListing.parse(body ?? Uint8List(0));
        } finally {
          await bulkSub.cancel();
        }
      });

  /// Download [path] from the device. [onProgress] reports 0..1.
  Future<Uint8List> downloadFile(String path,
          {void Function(double)? onProgress}) =>
      _serial(() async {
        await _ensureSubscribed();
        Uint8List? body;
        final bulkSub = _bulk.stream.listen((b) => body = b);
        final progSub = _bulkProgress.stream.listen((p) {
          if (onProgress != null && p.total > 0) {
            onProgress(p.recv / p.total);
          }
        });
        final readyF = _status.stream
            .firstWhere((e) =>
                e.status == FileOps.stReady ||
                e.status == FileOps.stError ||
                e.status == FileOps.stNotReady)
            .timeout(const Duration(seconds: 20));
        try {
          await _writeCtrl(_ctrlReq(FileOps.opDownload, Tlv.encode({1: path})));
          final ready = await readyF;
          if (ready.status == FileOps.stNotReady) {
            _raise(FileOps.errNotReady, notReady: true);
          }
          if (ready.status == FileOps.stError) _raise(ready.detail);
          final doneF = _status.stream
              .firstWhere((e) =>
                  e.status == FileOps.stDone || e.status == FileOps.stError)
              .timeout(const Duration(seconds: 120));
          final done = await doneF;
          if (done.status == FileOps.stError) _raise(done.detail);
          if (body == null || body!.isEmpty) _raise(FileOps.errIo);
          // body = [sub=0x02][file bytes] — strip the sub byte.
          return Uint8List.sublistView(body!, 1);
        } finally {
          await bulkSub.cancel();
          await progSub.cancel();
        }
      });

  /// Upload [bytes] to [path] on the device. [onProgress] reports 0..1.
  /// This is also the "push splash GIF" path: uploadFile('/vescfs/splash.gif', …).
  Future<void> uploadFile(String path, Uint8List bytes,
          {void Function(double)? onProgress}) =>
      _serial(() async {
        await _ensureSubscribed();
        final data = BleService.instance.fileDataChar;
        if (data == null) throw const FileOpException('files.err.noconn');
        final sha = Uint8List.fromList(sha256.convert(bytes).bytes);

        final readyF = _status.stream
            .firstWhere((e) =>
                e.status == FileOps.stReady ||
                e.status == FileOps.stError ||
                e.status == FileOps.stNotReady)
            .timeout(const Duration(seconds: 20));
        // BEGIN: [op][u32 len][32B sha][TLV {1:path}]
        final begin = BytesBuilder();
        begin.addByte(FileOps.opUpload);
        final lenb = ByteData(4)..setUint32(0, bytes.length, Endian.little);
        begin.add(lenb.buffer.asUint8List());
        begin.add(sha);
        begin.add(Tlv.encode({1: path}));
        await _writeCtrl(begin.takeBytes());

        final ready = await readyF;
        if (ready.status == FileOps.stNotReady) {
          _raise(FileOps.errNotReady, notReady: true);
        }
        if (ready.status == FileOps.stError) _raise(ready.detail);

        // Stream the bytes on the DATA characteristic.
        final mtu = BleService.instance.negotiatedMtu;
        final chunkSize = (mtu - 3).clamp(20, 244);
        final total = bytes.length;
        var off = 0;
        onProgress?.call(0);
        while (off < total) {
          if (!BleService.instance.isConnected) {
            throw const FileOpException('files.err.noconn');
          }
          final end = (off + chunkSize) < total ? off + chunkSize : total;
          await _writeData(data, Uint8List.sublistView(bytes, off, end));
          off = end;
          onProgress?.call(off / total);
        }

        final doneF = _status.stream
            .firstWhere((e) =>
                e.status == FileOps.stDone || e.status == FileOps.stError)
            .timeout(const Duration(seconds: 180));
        await _writeCtrl(_ctrlReq(FileOps.opUpEnd));
        final done = await doneF;
        if (done.status == FileOps.stError) _raise(done.detail);
      });

  Future<void> deleteEntry(String path) =>
      _simpleOp(_ctrlReq(FileOps.opDelete, Tlv.encode({1: path})));

  Future<void> mkdir(String path) =>
      _simpleOp(_ctrlReq(FileOps.opMkdir, Tlv.encode({1: path})));

  Future<void> rename(String src, String dst) =>
      _simpleOp(_ctrlReq(FileOps.opRename, Tlv.encode({1: src, 2: dst})));

  /// One-shot op that just awaits DONE/ERROR (delete/mkdir/rename).
  Future<void> _simpleOp(Uint8List req) => _serial(() async {
        await _ensureSubscribed();
        final doneF = _status.stream
            .firstWhere((e) =>
                e.status == FileOps.stDone ||
                e.status == FileOps.stError ||
                e.status == FileOps.stNotReady)
            .timeout(const Duration(seconds: 20));
        await _writeCtrl(req);
        final r = await doneF;
        if (r.status == FileOps.stNotReady) _raise(FileOps.errNotReady, notReady: true);
        if (r.status == FileOps.stError) _raise(r.detail);
      });

  /// Best-effort cancel of an in-flight transfer.
  Future<void> abort() async {
    try {
      await _writeCtrl(_ctrlReq(FileOps.opAbort));
    } catch (_) {}
  }

  /// One DATA chunk with the same busy-backoff as BleService._writeOtaData.
  Future<void> _writeData(BluetoothCharacteristic ch, Uint8List chunk) async {
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
}
