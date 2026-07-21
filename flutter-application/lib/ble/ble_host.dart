/// Background-isolate host for everything BLE.
///
/// flutter_foreground_task spins up a separate FlutterEngine + isolate for the
/// foreground service and runs [startBleTask] in it. Putting the whole BLE
/// stack here — connection lifecycle ([BleService]), the notification/media
/// pump ([Coordinator]), the file manager and OTA — means the link survives
/// the UI being backgrounded or killed: the service (and this isolate) keep
/// running. The UI talks to us over the data port (see ble_proxy.dart / ipc.dart).
///
/// Hard constraint: the port JSON-round-trips its payload, so every value sent
/// here must be JSON-safe (ints/doubles/bools/Strings/Lists/Maps). Binary
/// (firmware image, file bytes, icon PNGs) travels as base64 Strings or via a
/// temp-file path.
library;

import 'dart:async';
import 'dart:convert';
import 'dart:io';
import 'dart:typed_data';

import 'package:crypto/crypto.dart' show sha256;
import 'package:flutter_foreground_task/flutter_foreground_task.dart';
import 'package:path_provider/path_provider.dart';

import '../coordinator.dart';
import '../firmware/firmware_updater.dart';
import 'ble_service.dart';
import 'file_manager.dart';
import 'file_ops.dart';
import 'ipc.dart';
import 'vesc/vesc_link.dart';

/// Entry point executed in the foreground-service isolate. Must be a top-level
/// (or static) function annotated for the AOT tree-shaker.
@pragma('vm:entry-point')
void startBleTask() {
  FlutterForegroundTask.setTaskHandler(BleTaskHandler());
}

class BleTaskHandler extends TaskHandler {
  final _ble = BleService.instance;
  StreamSubscription<BleConnState>? _stateSub;
  Timer? _statsTimer;
  bool _statsInFlight = false;

  @override
  Future<void> onStart(DateTime timestamp, TaskStarter starter) async {
    // Forward every connection-state change to the UI (and keep the
    // notification text in sync — BleService already does that via its own
    // ForegroundBridge calls).
    _stateSub = _ble.state.listen(_pushState);
    // The pump: native notification/media events → BLE. Lives here now.
    await Coordinator.instance.start();
    // Re-arm auto-connect to the saved head unit.
    await _ble.resumeIfPaired();
    _pushState(_ble.currentState);
  }

  @override
  void onReceiveData(Object data) {
    if (data is Map) {
      _dispatch(Map<String, dynamic>.from(data));
    }
  }

  @override
  void onRepeatEvent(DateTime timestamp) {}

  @override
  Future<void> onDestroy(DateTime timestamp) async {
    _statsTimer?.cancel();
    await _stateSub?.cancel();
  }

  // ---- state push ----

  Map<String, dynamic> _statusMap() => {
        'state': _ble.currentState.name,
        'savedRemoteId': _ble.savedRemoteId,
        'supportsFm': _ble.supportsFileManager,
        'supportsOta': _ble.supportsOta,
        'supportsBleOta': _ble.supportsBleOta,
        'supportsLisp': _ble.supportsLisp,
        'mtu': _ble.negotiatedMtu,
      };

  void _pushState(BleConnState s) {
    // Stop stats polling if the link drops — VescLink would just time out.
    if (s != BleConnState.connected) {
      _statsTimer?.cancel();
      _statsTimer = null;
    }
    FlutterForegroundTask.sendDataToMain({'t': IpcEvt.state, ..._statusMap()});
  }

  // ---- command dispatch ----

  Future<void> _dispatch(Map<String, dynamic> m) async {
    final id = m['id'] as int?;
    final cmd = m['cmd'] as String?;
    try {
      switch (cmd) {
        case IpcCmd.getStatus:
          _reply(id, _statusMap());
          break;

        case IpcCmd.scan:
          final results = await _ble.scan();
          _reply(id, {
            'devices': [
              for (final r in results)
                {
                  'remoteId': r.device.remoteId.str,
                  'name': r.advertisementData.advName.isNotEmpty
                      ? r.advertisementData.advName
                      : r.device.platformName,
                  'rssi': r.rssi,
                }
            ],
          });
          break;

        case IpcCmd.connect:
          await _ble.connectById(m['remoteId'] as String);
          _reply(id, {});
          break;

        case IpcCmd.forget:
          await _ble.forget();
          _reply(id, {});
          break;

        case IpcCmd.bleRestart:
          await _ble.restart();
          _reply(id, {});
          break;

        case IpcCmd.send:
          // Fire-and-forget; no reply.
          await _ble.sendRaw(
              m['kind'] as String, base64Decode(m['b64'] as String));
          break;

        case IpcCmd.readOtaInfo:
          final info = await _ble.readOtaInfo();
          _reply(id, {
            'info': info == null
                ? null
                : {
                    'ip': info.ip,
                    'port': info.port,
                    'ssid': info.ssid,
                    'password': info.password,
                    'version': info.version,
                    'model': info.model,
                  },
          });
          break;

        case IpcCmd.bleOta:
          await _doBleOta(id, m['model'] as String?);
          break;

        case IpcCmd.reloadFilter:
          await Coordinator.instance.reloadFilter();
          break;

        case IpcCmd.fileList:
          final listing = await FileManager.instance.listDir(m['path'] as String);
          _reply(id, {
            'truncated': listing.truncated,
            'entries': [
              for (final e in listing.entries)
                {
                  'name': e.name,
                  'isDir': e.isDir,
                  'size': e.size,
                  'mtime': e.mtime,
                }
            ],
          });
          break;

        case IpcCmd.fileDownload:
          await _doDownload(id, m['path'] as String);
          break;

        case IpcCmd.fileUpload:
          await FileManager.instance.uploadFile(
              m['path'] as String, base64Decode(m['b64'] as String),
              onProgress: (f) => _progress(id, f));
          _reply(id, {});
          break;

        case IpcCmd.fileUploadPath:
          final bytes = await File(m['src'] as String).readAsBytes();
          await FileManager.instance.uploadFile(m['path'] as String, bytes,
              onProgress: (f) => _progress(id, f));
          _reply(id, {});
          break;

        case IpcCmd.fileDelete:
          await FileManager.instance.deleteEntry(m['path'] as String);
          _reply(id, {});
          break;

        case IpcCmd.fileMkdir:
          await FileManager.instance.mkdir(m['path'] as String);
          _reply(id, {});
          break;

        case IpcCmd.fileRename:
          await FileManager.instance
              .rename(m['src'] as String, m['dst'] as String);
          _reply(id, {});
          break;

        case IpcCmd.lispRead:
          final code = await VescLink.instance
              .readCode(onProgress: (f) => _progress(id, f));
          _reply(id, {'code': code});
          break;

        case IpcCmd.lispUpload:
          await VescLink.instance.uploadCode(m['code'] as String,
              run: m['run'] as bool? ?? false,
              onProgress: (f) => _progress(id, f));
          _reply(id, {});
          break;

        case IpcCmd.lispRun:
          await VescLink.instance.setRunning(true);
          _reply(id, {});
          break;

        case IpcCmd.lispStop:
          await VescLink.instance.setRunning(false);
          _reply(id, {});
          break;

        case IpcCmd.lispStatsStart:
          _startStatsPolling();
          _reply(id, {});
          break;

        case IpcCmd.lispStatsStop:
          _statsTimer?.cancel();
          _statsTimer = null;
          _reply(id, {});
          break;
      }
    } on VescLispException catch (e) {
      _replyError(id, kind: IpcErrKind.lispOp, key: e.key);
    } on FileOpException catch (e) {
      _replyError(id,
          kind: IpcErrKind.fileOp, key: e.key, notReady: e.notReady);
    } catch (e) {
      _replyError(id, kind: IpcErrKind.generic, msg: '$e');
    }
  }

  /// Poll LISP runtime stats (~2.5 Hz) and stream each snapshot to the UI while
  /// the Variables tab is open. Ticks that overlap a slow reply are skipped;
  /// errors (e.g. transient link loss) are swallowed so the timer keeps going.
  void _startStatsPolling() {
    _statsTimer?.cancel();
    _statsTimer =
        Timer.periodic(const Duration(milliseconds: 400), (_) async {
      if (_statsInFlight) return;
      _statsInFlight = true;
      try {
        final s = await VescLink.instance.getStats(all: true);
        FlutterForegroundTask
            .sendDataToMain({'t': IpcEvt.lispStats, 'stats': s.toMap()});
      } catch (_) {
        // ignore — next tick retries
      } finally {
        _statsInFlight = false;
      }
    });
  }

  Future<void> _doBleOta(int? id, String? model) async {
    try {
      final image = await FirmwareUpdater.imageFor(model);
      final digest = Uint8List.fromList(sha256.convert(image).bytes);
      final res = await _ble.bleOta(
        image,
        digest,
        onUpload: (f) => FlutterForegroundTask
            .sendDataToMain({'t': IpcEvt.otaUpload, 'id': id, 'frac': f}),
        onVerify: () => FlutterForegroundTask
            .sendDataToMain({'t': IpcEvt.otaVerify, 'id': id}),
      );
      _reply(id, {'ok': res.ok, 'errorKey': res.errorKey});
    } catch (e) {
      _reply(id, {'ok': false, 'errorKey': 'fw.ota.bleerror'});
    }
  }

  Future<void> _doDownload(int? id, String path) async {
    final bytes = await FileManager.instance
        .downloadFile(path, onProgress: (f) => _progress(id, f));
    // Hand the bytes to the UI via a temp file — the port can't carry a
    // multi-MB blob efficiently. The UI reads + deletes it.
    final dir = await getTemporaryDirectory();
    final name = path.split('/').isNotEmpty ? path.split('/').last : 'download';
    final tmp = File('${dir.path}/dl_${id ?? 0}_$name');
    await tmp.writeAsBytes(bytes);
    _reply(id, {'tmpPath': tmp.path});
  }

  // ---- port helpers ----

  void _reply(int? id, Map<String, dynamic> payload) {
    if (id == null) return;
    FlutterForegroundTask
        .sendDataToMain({'t': IpcEvt.resp, 'id': id, ...payload});
  }

  void _replyError(int? id,
      {required String kind, String? key, bool notReady = false, String? msg}) {
    if (id == null) return;
    FlutterForegroundTask.sendDataToMain({
      't': IpcEvt.err,
      'id': id,
      'kind': kind,
      'key': key,
      'notReady': notReady,
      'msg': msg,
    });
  }

  void _progress(int? id, double frac) {
    if (id == null) return;
    FlutterForegroundTask
        .sendDataToMain({'t': IpcEvt.progress, 'id': id, 'frac': frac});
  }
}
