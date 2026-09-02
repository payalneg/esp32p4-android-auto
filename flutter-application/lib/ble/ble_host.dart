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
import '../helper/helper_protocol.dart';
import '../helper/helper_service.dart';
import 'ble_service.dart';
import 'file_manager.dart';
import 'file_ops.dart';
import 'ipc.dart';
import 'lisp_models.dart';
import 'uuids.dart';
import 'vesc/vesc_link.dart';
import 'vesc/vesc_target.dart';

/// Entry point executed in the foreground-service isolate. Must be a top-level
/// (or static) function annotated for the AOT tree-shaker.
@pragma('vm:entry-point')
void startBleTask() {
  FlutterForegroundTask.setTaskHandler(BleTaskHandler());
}

class BleTaskHandler extends TaskHandler {
  final _ble = BleService.instance;
  StreamSubscription<BleConnState>? _stateSub;
  StreamSubscription<VescTargetInfo>? _targetSub;
  Timer? _statsTimer;
  bool _statsInFlight = false;
  StreamSubscription<LispConsoleLine>? _consoleSub;
  final _helperSubs = <StreamSubscription<dynamic>>[];
  final _consoleBatch = <LispConsoleLine>[];
  Timer? _consoleTimer;
  bool _consolePushOn = false;

  @override
  Future<void> onStart(DateTime timestamp, TaskStarter starter) async {
    // Forward every connection-state change to the UI (and keep the
    // notification text in sync — BleService already does that via its own
    // ForegroundBridge calls).
    _stateSub = _ble.state.listen(_pushState);
    // Which NUS link the LISP editor talks to (head unit bridge / a direct
    // adapter). Restored, not connected — the adapter link is only held while
    // the editor is open.
    await VescTarget.instance.restore();
    _targetSub = VescTarget.instance.changes.listen(_pushTarget);
    _wireHelper();
    // Script output accumulates in VescLink's ring whether anyone is watching
    // or not; we only forward it while a UI panel has asked for the live feed.
    _consoleSub = VescLink.instance.prints.listen((l) {
      if (_consolePushOn) _consoleBatch.add(l);
    });
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
    _consoleTimer?.cancel();
    await _stateSub?.cancel();
    await _targetSub?.cancel();
    await _consoleSub?.cancel();
    for (final s in _helperSubs) {
      await s.cancel();
    }
    _helperSubs.clear();
    await HelperService.instance.disconnect();
  }

  // ---- state push ----

  Map<String, dynamic> _statusMap() => {
        'state': _ble.currentState.name,
        'savedRemoteId': _ble.savedRemoteId,
        'supportsFm': _ble.supportsFileManager,
        'supportsOta': _ble.supportsOta,
        'supportsBleOta': _ble.supportsBleOta,
        'mtu': _ble.negotiatedMtu,
        // Whether the head unit exposes the NUS bridge is part of the VESC
        // target info now (`headUnitAvailable`) — one source of truth for the
        // LISP editor, which may instead be riding a direct adapter.
        'vescTarget': VescTarget.instance.info.toMap(),
      };

  void _pushState(BleConnState s) {
    // Head-unit link changed: if that's what the LISP editor is riding on,
    // stats polling would just time out — stop it.
    if (s != BleConnState.connected &&
        VescTarget.instance.kind == VescTargetKind.headUnit) {
      _stopStatsPolling();
    }
    FlutterForegroundTask.sendDataToMain({'t': IpcEvt.state, ..._statusMap()});
    // The head unit going up/down also changes whether it's a usable target.
    _pushTarget(VescTarget.instance.info);
  }

  void _pushTarget(VescTargetInfo info) {
    if (!info.connected && VescTarget.instance.kind == VescTargetKind.direct) {
      _stopStatsPolling();
    }
    FlutterForegroundTask
        .sendDataToMain({'t': IpcEvt.vescTarget, 'target': info.toMap()});
  }

  void _stopStatsPolling() {
    _statsTimer?.cancel();
    _statsTimer = null;
  }

  // ---- helper (ESP32-C3) ----

  Map<String, dynamic> _helperStatusMap() => {
        'state': HelperService.instance.connState.name,
        'fw': HelperService.instance.fwVersion,
        'busy': HelperService.instance.busy,
      };

  /// Fan the helper's streams out to the UI. Subscribed for the lifetime of
  /// the isolate: they only carry traffic while a helper link is up, and the
  /// UI needs the state change that brings it up.
  void _wireHelper() {
    _helperSubs.addAll([
      HelperService.instance.stateChanges.listen((_) => FlutterForegroundTask
          .sendDataToMain({'t': IpcEvt.helperState, ..._helperStatusMap()})),
      HelperService.instance.statusUpdates.listen((s) => FlutterForegroundTask
          .sendDataToMain(
              {'t': IpcEvt.helperStatusFrame, 'status': s.toMap()})),
      HelperService.instance.paramUpdates.listen((p) => FlutterForegroundTask
          .sendDataToMain({'t': IpcEvt.helperParams, 'b64': p.toB64()})),
      HelperService.instance.bindingUpdates.listen((b) => FlutterForegroundTask
          .sendDataToMain({'t': IpcEvt.helperBinding, 'binding': b.toMap()})),
      HelperService.instance.scanHits.listen((h) => FlutterForegroundTask
          .sendDataToMain({'t': IpcEvt.helperScanHit, 'hit': h.toMap()})),
      HelperService.instance.logLines.listen((l) => FlutterForegroundTask
          .sendDataToMain({'t': IpcEvt.helperLog, 'line': l})),
      HelperService.instance.otaProgress.listen((f) => FlutterForegroundTask
          .sendDataToMain({'t': IpcEvt.helperOtaProgress, 'frac': f})),
    ]);
  }

  /// Turn the live console feed on/off. Lines are batched rather than pushed
  /// one per port message: a script printing at 10 Hz would otherwise mean 10
  /// JSON round-trips a second for a handful of bytes each.
  void _setConsolePush(bool on) {
    _consolePushOn = on;
    _consoleTimer?.cancel();
    _consoleTimer = null;
    _consoleBatch.clear();
    if (!on) return;
    _consoleTimer = Timer.periodic(const Duration(milliseconds: 200), (_) {
      if (_consoleBatch.isEmpty) return;
      final chunk = LispConsoleChunk(
        lines: List.of(_consoleBatch),
        nextSeq: _consoleBatch.last.seq + 1,
        dropped: VescLink.instance.console.dropped,
        alive: VescLink.instance.console.everSawPrint,
      );
      _consoleBatch.clear();
      FlutterForegroundTask
          .sendDataToMain({'t': IpcEvt.lispConsole, 'chunk': chunk.toMap()});
    });
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
          // `quiet` (LISP adapter picker) keeps the scan from overwriting the
          // head unit's connection state in the UI.
          final quiet = m['quiet'] as bool? ?? false;
          final results = await _ble.scan(mutateState: !quiet);
          _reply(id, {
            'devices': [
              for (final r in results)
                {
                  'remoteId': r.device.remoteId.str,
                  'name': r.advertisementData.advName.isNotEmpty
                      ? r.advertisementData.advName
                      : r.device.platformName,
                  'rssi': r.rssi,
                  // Advertises the Nordic UART Service → a candidate VESC
                  // adapter. Not conclusive (some adapters don't put it in the
                  // adv packet), just how the picker sorts.
                  'nus': r.advertisementData.serviceUuids.any((g) =>
                      g.toString().toLowerCase() == NusUuids.service),
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
          final r = await VescLink.instance
              .readCode(onProgress: (f) => _progress(id, f));
          _reply(id, {'code': r.code, 'stats': r.stats.toMap()});
          break;

        case IpcCmd.lispUpload:
          final st = await VescLink.instance.uploadCode(m['code'] as String,
              run: m['run'] as bool? ?? false,
              stopFirst: m['stopFirst'] as bool? ?? false,
              onProgress: (f) => _progress(id, f));
          _reply(id, {'stats': st.toMap()});
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
          _stopStatsPolling();
          _reply(id, {});
          break;

        case IpcCmd.lispStatsOnce:
          final s = await VescLink.instance
              .getStats(all: m['all'] as bool? ?? true);
          _reply(id, {'stats': s.toMap()});
          break;

        case IpcCmd.lispConsoleRead:
          final chunk = VescLink.instance.console.read(
            sinceSeq: (m['sinceSeq'] as num?)?.toInt() ?? 0,
            maxLines: (m['maxLines'] as num?)?.toInt() ?? 200,
          );
          _reply(id, {'chunk': chunk.toMap()});
          break;

        case IpcCmd.lispConsoleClear:
          VescLink.instance.console.clear();
          _consoleBatch.clear();
          _reply(id, {});
          break;

        case IpcCmd.lispConsoleSub:
          _setConsolePush(m['on'] as bool? ?? false);
          _reply(id, {});
          break;

        case IpcCmd.lispConsoleDebug:
          VescLink.instance.consoleDebug = m['on'] as bool? ?? false;
          _reply(id, {
            'histogram': {
              for (final e in VescLink.instance.unmatchedHistogram.entries)
                '${e.key}': e.value
            }
          });
          break;

        case IpcCmd.helperConnect:
          await HelperService.instance
              .connect(remoteId: m['remoteId'] as String?);
          _reply(id, _helperStatusMap());
          break;

        case IpcCmd.helperDisconnect:
          await HelperService.instance.disconnect();
          _reply(id, _helperStatusMap());
          break;

        case IpcCmd.helperStatus:
          _reply(id, _helperStatusMap());
          break;

        case IpcCmd.helperScan:
          await HelperService.instance
              .scanRemotes((m['what'] as num?)?.toInt() ?? kWhatButton);
          _reply(id, {});
          break;

        case IpcCmd.helperBind:
          await HelperService.instance.bind(
              ScanHit.fromMap(Map<String, dynamic>.from(m['hit'] as Map)));
          _reply(id, {});
          break;

        case IpcCmd.helperUnbind:
          await HelperService.instance
              .unbind((m['what'] as num?)?.toInt() ?? kWhatButton);
          _reply(id, {});
          break;

        case IpcCmd.helperGetParams:
          await HelperService.instance.getParams();
          _reply(id, {});
          break;

        case IpcCmd.helperSetParams:
          await HelperService.instance
              .setParams(base64Decode(m['b64'] as String));
          _reply(id, {});
          break;

        case IpcCmd.helperThrottle:
          await HelperService.instance
              .setThrottle((m['value'] as num?)?.toInt() ?? 0);
          _reply(id, {});
          break;

        case IpcCmd.helperGetBinding:
          await HelperService.instance
              .getBinding((m['idx'] as num?)?.toInt() ?? 0);
          _reply(id, {});
          break;

        case IpcCmd.helperSetBinding:
          await HelperService.instance.setBinding(ButtonBinding.fromMap(
              Map<String, dynamic>.from(m['binding'] as Map)));
          _reply(id, {});
          break;

        case IpcCmd.helperOta:
          await HelperService.instance
              .flashFirmware(base64Decode(m['b64'] as String));
          _reply(id, {});
          break;

        case IpcCmd.vescSelect:
          if (m['kind'] == VescTargetKind.direct.name) {
            await VescTarget.instance.selectDirect(
                m['remoteId'] as String, m['name'] as String? ?? '');
          } else {
            await VescTarget.instance.selectHeadUnit();
          }
          _reply(id, {'target': VescTarget.instance.info.toMap()});
          break;

        case IpcCmd.vescResume:
          await VescTarget.instance.resume();
          _reply(id, {'target': VescTarget.instance.info.toMap()});
          break;

        case IpcCmd.vescRelease:
          _stopStatsPolling();
          await VescTarget.instance.release();
          _reply(id, {'target': VescTarget.instance.info.toMap()});
          break;
      }
    } on VescLispException catch (e) {
      _replyError(id, kind: IpcErrKind.lispOp, key: e.key);
    } on HelperException catch (e) {
      _replyError(id, kind: IpcErrKind.helperOp, msg: e.message);
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
      // Progress fires once per BLE chunk — ~9 000 times per image. Each
      // IPC hop to the UI isolate is a platform-channel message and a
      // rebuild; throttle to visible steps so the sender loop isn't spending
      // its time between writes on progress plumbing.
      var lastSent = -1.0;
      var lastAt = DateTime.fromMillisecondsSinceEpoch(0);
      final res = await _ble.bleOta(
        image,
        digest,
        onUpload: (f) {
          final now = DateTime.now();
          if (f < 1.0 &&
              f - lastSent < 0.005 &&
              now.difference(lastAt).inMilliseconds < 250) {
            return;
          }
          lastSent = f;
          lastAt = now;
          FlutterForegroundTask
              .sendDataToMain({'t': IpcEvt.otaUpload, 'id': id, 'frac': f});
        },
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
