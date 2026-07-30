/// UI-isolate proxy for the BLE stack that now lives in the foreground-service
/// task isolate (see ble_host.dart). Mirrors the slice of the old
/// `BleService` / `FileManager` API the screens use, marshalling each call over
/// the flutter_foreground_task data port.
///
/// All payloads are JSON-safe; binary travels as base64 (see ipc.dart).
library;

import 'dart:async';
import 'dart:convert';
import 'dart:io';
import 'dart:typed_data';

import 'package:flutter_foreground_task/flutter_foreground_task.dart';
import 'package:shared_preferences/shared_preferences.dart';

import '../firmware/ota_info.dart';
import 'ble_service.dart' show BleConnState;
import 'file_ops.dart';
import 'ipc.dart';
import 'lisp_models.dart';
import 'messages.dart';

export 'ble_service.dart' show BleConnState;

/// A scan hit, stripped to the JSON-safe fields the pairing UI needs (the
/// flutter_blue_plus ScanResult/BluetoothDevice objects can't cross isolates).
class ScanDevice {
  final String remoteId;
  final String name;
  final int rssi;

  /// Advertises the Nordic UART Service — a likely VESC adapter. The LISP
  /// editor's adapter picker lists these first. Not conclusive: an adapter may
  /// keep NUS out of its adv packet and still expose it after connecting.
  final bool nus;
  const ScanDevice(this.remoteId, this.name, this.rssi, {this.nus = false});
}

/// Error surfaced from the task isolate over the port.
class IpcException implements Exception {
  final String kind;
  final String? msg;
  IpcException(this.kind, this.msg);
  @override
  String toString() => 'IpcException($kind, $msg)';
}

/// A LISP-editor operation failed; [key] is an i18n key the UI localizes.
class LispException implements Exception {
  final String key;
  const LispException(this.key);
  @override
  String toString() => 'LispException($key)';
}

/// A helper (ESP32-C3) operation failed. Carries a raw message rather than an
/// i18n key: these are protocol-level ("missing characteristic …") and are
/// shown verbatim in the helper's log panel.
class HelperOpException implements Exception {
  final String message;
  const HelperOpException(this.message);
  @override
  String toString() => message;
}

class BleProxy {
  BleProxy._();
  static final BleProxy instance = BleProxy._();

  // Mirror of BleService._prefSavedId — read directly so the UI can show the
  // saved device at first paint, before the task pushes its first state.
  static const _prefSavedId = 'ble_saved_remote_id_v1';

  final _stateCtrl = StreamController<BleConnState>.broadcast();
  final _lispStatsCtrl = StreamController<LispStats>.broadcast();
  final _lispConsoleCtrl = StreamController<LispConsoleLine>.broadcast();

  /// Raw helper events, re-typed by [HelperProxy] — keeping them untyped here
  /// means the BLE proxy doesn't need to know the helper's protocol.
  final _helperCtrl = StreamController<Map<String, dynamic>>.broadcast();
  Stream<Map<String, dynamic>> get helperEvents => _helperCtrl.stream;
  final _vescTargetCtrl = StreamController<VescTargetInfo>.broadcast();
  BleConnState _state = BleConnState.idle;
  String? _savedRemoteId;
  bool _supportsFm = false;
  bool _supportsOta = false;
  bool _supportsBleOta = false;
  int _mtu = 247;
  VescTargetInfo _vescTarget = const VescTargetInfo(
      kind: VescTargetKind.headUnit, state: VescLinkState.idle);

  int _nextId = 1;
  final _pending = <int, Completer<Map<String, dynamic>>>{};
  final _onProgress = <int, void Function(double)>{};
  final _onOtaUpload = <int, void Function(double)>{};
  final _onOtaVerify = <int, void Function()>{};
  bool _wired = false;
  bool _syncing = false;

  Stream<BleConnState> get state => _stateCtrl.stream;
  Stream<LispStats> get lispStats => _lispStatsCtrl.stream;

  /// Live `(print ...)` output from the script running on the VESC. Only
  /// flows while someone has called [LispProxy.subscribeConsole].
  Stream<LispConsoleLine> get lispConsole => _lispConsoleCtrl.stream;

  /// Which NUS link the LISP editor is pointed at, and its state.
  Stream<VescTargetInfo> get vescTarget => _vescTargetCtrl.stream;
  VescTargetInfo get currentVescTarget => _vescTarget;
  BleConnState get currentState => _state;
  String? get savedRemoteId => _savedRemoteId;
  bool get supportsFileManager => _supportsFm;
  bool get supportsOta => _supportsOta;
  bool get supportsBleOta => _supportsBleOta;
  int get negotiatedMtu => _mtu;

  /// Wire up the port callback and prime the saved-device id from prefs. Call
  /// once at app start, before the foreground service is started.
  Future<void> init() async {
    if (!_wired) {
      _wired = true;
      FlutterForegroundTask.addTaskDataCallback(_onData);
    }
    final p = await SharedPreferences.getInstance();
    _savedRemoteId = p.getString(_prefSavedId);
    // Pull the live status — the task only pushes on *change*, so without this
    // a fresh UI isolate would paint "not connected" over a working link.
    unawaited(syncStatus());
  }

  /// Ask the task isolate for its current status and apply it locally.
  ///
  /// The BLE stack lives in the foreground-service isolate and only *pushes*
  /// state when it changes ([BleTaskHandler] `_pushState`); its one unsolicited
  /// push happens in `onStart`, once per service lifetime. So whenever the UI
  /// isolate is younger than the service — app reopened after Android killed
  /// the UI, service auto-started on boot, or simply the first launch where the
  /// push raced our port callback — the UI has never seen a state message and
  /// sits on [BleConnState.idle]: the status card reads "not connected" and the
  /// Files / LISP entries stay hidden while the link is in fact up and pumping.
  /// Pulling the status closes that gap.
  ///
  /// Retries with a backoff because the service may still be starting when this
  /// first runs (init() is called before `ForegroundBridge.start()`).
  Future<void> syncStatus() async {
    if (_syncing) return;
    _syncing = true;
    try {
      var delayMs = 400;
      for (var attempt = 0; attempt < 6; attempt++) {
        if (await _askStatus()) return;
        await Future<void>.delayed(Duration(milliseconds: delayMs));
        delayMs *= 2;
      }
    } finally {
      _syncing = false;
    }
  }

  /// One [IpcCmd.getStatus] round-trip. Returns false (and leaves the local
  /// state untouched) if the service isn't up yet or doesn't answer in time.
  Future<bool> _askStatus() async {
    if (!await FlutterForegroundTask.isRunningService) return false;
    final id = _nextId++;
    final c = Completer<Map<String, dynamic>>();
    _pending[id] = c;
    FlutterForegroundTask.sendDataToTask({'cmd': IpcCmd.getStatus, 'id': id});
    try {
      _applyState(await c.future.timeout(const Duration(seconds: 2)));
      return true;
    } catch (_) {
      return false;
    } finally {
      _pending.remove(id);
    }
  }

  // ---- inbound events ----

  void _onData(Object data) {
    if (data is! Map) return;
    final m = Map<String, dynamic>.from(data);
    switch (m['t']) {
      case IpcEvt.state:
        _applyState(m);
        break;
      case IpcEvt.resp:
        final id = m['id'] as int?;
        _forget(id);
        _pending.remove(id)?.complete(m);
        break;
      case IpcEvt.err:
        final id = m['id'] as int?;
        _forget(id);
        _pending.remove(id)?.completeError(_errorFrom(m));
        break;
      case IpcEvt.progress:
        _onProgress[m['id']]?.call((m['frac'] as num).toDouble());
        break;
      case IpcEvt.otaUpload:
        _onOtaUpload[m['id']]?.call((m['frac'] as num).toDouble());
        break;
      case IpcEvt.otaVerify:
        _onOtaVerify[m['id']]?.call();
        break;
      case IpcEvt.lispStats:
        final s = m['stats'];
        if (s is Map) {
          _lispStatsCtrl.add(LispStats.fromMap(Map<String, dynamic>.from(s)));
        }
        break;
      case IpcEvt.lispConsole:
        final c = m['chunk'];
        if (c is Map) {
          final chunk =
              LispConsoleChunk.fromMap(Map<String, dynamic>.from(c));
          for (final l in chunk.lines) {
            _lispConsoleCtrl.add(l);
          }
        }
        break;
      case IpcEvt.vescTarget:
        _applyVescTarget(m['target']);
        break;
      case IpcEvt.helperState:
      case IpcEvt.helperStatusFrame:
      case IpcEvt.helperParams:
      case IpcEvt.helperBinding:
      case IpcEvt.helperScanHit:
      case IpcEvt.helperLog:
      case IpcEvt.helperOtaProgress:
        _helperCtrl.add(m);
        break;
    }
  }

  void _applyVescTarget(Object? raw) {
    if (raw is! Map) return;
    _vescTarget = VescTargetInfo.fromMap(Map<String, dynamic>.from(raw));
    _vescTargetCtrl.add(_vescTarget);
  }

  void _forget(int? id) {
    if (id == null) return;
    _onProgress.remove(id);
    _onOtaUpload.remove(id);
    _onOtaVerify.remove(id);
  }

  Object _errorFrom(Map<String, dynamic> m) {
    if (m['kind'] == IpcErrKind.fileOp) {
      return FileOpException(m['key'] as String? ?? 'files.err.unknown',
          notReady: m['notReady'] as bool? ?? false);
    }
    if (m['kind'] == IpcErrKind.lispOp) {
      return LispException(m['key'] as String? ?? 'lisp.err.unknown');
    }
    if (m['kind'] == IpcErrKind.helperOp) {
      return HelperOpException(m['msg'] as String? ?? 'helper error');
    }
    return IpcException(m['kind'] as String? ?? 'generic', m['msg'] as String?);
  }

  void _applyState(Map<String, dynamic> m) {
    _savedRemoteId = m['savedRemoteId'] as String?;
    _supportsFm = m['supportsFm'] as bool? ?? false;
    _supportsOta = m['supportsOta'] as bool? ?? false;
    _supportsBleOta = m['supportsBleOta'] as bool? ?? false;
    _mtu = (m['mtu'] as num?)?.toInt() ?? 247;
    _applyVescTarget(m['vescTarget']);
    final name = m['state'] as String?;
    _state = BleConnState.values.firstWhere((e) => e.name == name,
        orElse: () => BleConnState.idle);
    _stateCtrl.add(_state);
  }

  // ---- outbound requests ----

  Future<Map<String, dynamic>> _request(
    String cmd, [
    Map<String, dynamic>? args,
  ]) {
    final id = _nextId++;
    final c = Completer<Map<String, dynamic>>();
    _pending[id] = c;
    FlutterForegroundTask.sendDataToTask({'cmd': cmd, 'id': id, ...?args});
    return c.future;
  }

  void _fireAndForget(String cmd, [Map<String, dynamic>? args]) {
    FlutterForegroundTask.sendDataToTask({'cmd': cmd, ...?args});
  }

  /// [quiet] scans without touching the head unit's connection state — used by
  /// the LISP adapter picker, which may run while a head unit is connected.
  Future<List<ScanDevice>> scan({bool quiet = false}) async {
    final r = await _request(IpcCmd.scan, {'quiet': quiet});
    final list = (r['devices'] as List?) ?? const [];
    return [
      for (final d in list)
        ScanDevice(d['remoteId'] as String, d['name'] as String? ?? '',
            (d['rssi'] as num?)?.toInt() ?? 0,
            nus: d['nus'] as bool? ?? false)
    ];
  }

  Future<void> connect(String remoteId) =>
      _request(IpcCmd.connect, {'remoteId': remoteId});

  Future<void> forget() => _request(IpcCmd.forget);

  /// Force a clean BLE reconnect of the saved device (flush GATT cache +
  /// re-discover) to unstick a wedged link. No-op if nothing is paired.
  Future<void> restartBle() => _request(IpcCmd.bleRestart);

  void reloadFilter() => _fireAndForget(IpcCmd.reloadFilter);

  Future<OtaInfo?> readOtaInfo() async {
    final r = await _request(IpcCmd.readOtaInfo);
    final info = r['info'] as Map?;
    if (info == null) return null;
    return OtaInfo(
      ip: info['ip'] as String? ?? '',
      port: (info['port'] as num?)?.toInt() ?? 0,
      ssid: info['ssid'] as String? ?? '',
      password: info['password'] as String? ?? '',
      version: info['version'] as String? ?? '',
      model: info['model'] as String?,
    );
  }

  Future<({bool ok, String? errorKey})> bleOta(
    String? model, {
    void Function(double)? onUpload,
    void Function()? onVerify,
  }) async {
    final id = _nextId;
    if (onUpload != null) _onOtaUpload[id] = onUpload;
    if (onVerify != null) _onOtaVerify[id] = onVerify;
    try {
      final r = await _request(IpcCmd.bleOta, {'model': model});
      return (ok: r['ok'] as bool? ?? false, errorKey: r['errorKey'] as String?);
    } catch (_) {
      return (ok: false, errorKey: 'fw.ota.blefail');
    }
  }

  // ---- fire-and-forget media / notification sends (test panel) ----

  Future<void> sendNotification(NotificationMsg n) async =>
      _sendRaw('notif', n.encode());
  Future<void> sendIcon(IconMsg i) async => _sendRaw('icon', i.encode());
  Future<void> sendMedia(MediaMsg m) async => _sendRaw('media', m.encode());
  Future<void> sendAlbumArt(IconMsg i) async => _sendRaw('albumArt', i.encode());

  void _sendRaw(String kind, Uint8List body) => _fireAndForget(
      IpcCmd.send, {'kind': kind, 'b64': base64Encode(body)});

  // ---- file manager helpers (used by FileManagerProxy) ----

  Future<Map<String, dynamic>> fileRequest(
    String cmd,
    Map<String, dynamic> args, {
    void Function(double)? onProgress,
  }) {
    final id = _nextId;
    if (onProgress != null) _onProgress[id] = onProgress;
    return _request(cmd, args);
  }
}

/// Drop-in for the old `FileManager.instance`, routed through [BleProxy].
class FileManagerProxy {
  FileManagerProxy._();
  static final FileManagerProxy instance = FileManagerProxy._();

  final _ble = BleProxy.instance;

  bool get supported => _ble.supportsFileManager;

  Future<RemoteListing> listDir(String path) async {
    final r = await _ble.fileRequest(IpcCmd.fileList, {'path': path});
    final entries = (r['entries'] as List?) ?? const [];
    return RemoteListing([
      for (final e in entries)
        RemoteEntry(
          name: e['name'] as String? ?? '',
          isDir: e['isDir'] as bool? ?? false,
          size: (e['size'] as num?)?.toInt() ?? 0,
          mtime: (e['mtime'] as num?)?.toInt() ?? 0,
        )
    ], r['truncated'] as bool? ?? false);
  }

  Future<Uint8List> downloadFile(String path,
      {void Function(double)? onProgress}) async {
    final r = await _ble
        .fileRequest(IpcCmd.fileDownload, {'path': path}, onProgress: onProgress);
    final tmp = File(r['tmpPath'] as String);
    final bytes = await tmp.readAsBytes();
    try {
      await tmp.delete();
    } catch (_) {}
    return bytes;
  }

  /// Upload in-memory bytes (e.g. generated splash frames).
  Future<void> uploadFile(String path, Uint8List bytes,
          {void Function(double)? onProgress}) =>
      _ble.fileRequest(IpcCmd.fileUpload,
          {'path': path, 'b64': base64Encode(bytes)}, onProgress: onProgress);

  /// Upload a file already on disk (the task reads it) — avoids shoving a big
  /// blob through the port.
  Future<void> uploadLocalFile(String devicePath, String localPath,
          {void Function(double)? onProgress}) =>
      _ble.fileRequest(IpcCmd.fileUploadPath,
          {'path': devicePath, 'src': localPath}, onProgress: onProgress);

  Future<void> deleteEntry(String path) =>
      _ble.fileRequest(IpcCmd.fileDelete, {'path': path});

  Future<void> mkdir(String path) =>
      _ble.fileRequest(IpcCmd.fileMkdir, {'path': path});

  Future<void> rename(String src, String dst) =>
      _ble.fileRequest(IpcCmd.fileRename, {'src': src, 'dst': dst});
}

/// UI-facing handle for the LISP editor, routed through [BleProxy] to the
/// [VescLink] running in the background isolate. Read/upload/run/stop the
/// VESC's LISP script and stream its runtime variables. Throws
/// [LispException] (i18n key) on device-reported errors.
///
/// The link it rides on is whatever [VescTargetInfo] says: the head unit's NUS
/// bridge, or a stand-alone VESC BLE adapter the app connects to itself
/// ([selectDirect]) — the latter works with no head unit at all.
class LispProxy {
  LispProxy._();
  static final LispProxy instance = LispProxy._();

  final _ble = BleProxy.instance;

  /// Whether a LISP operation could run right now (target link is up).
  bool get supported => _ble.currentVescTarget.connected;

  /// Current target + its link state, and a stream of changes.
  VescTargetInfo get target => _ble.currentVescTarget;
  Stream<VescTargetInfo> get targets => _ble.vescTarget;

  /// Live LISP runtime-stats snapshots while [startStats] is active.
  Stream<LispStats> get stats => _ble.lispStats;

  /// Read the current LISP script text from the VESC, with the timings of the
  /// transfer (see [VescXferStats]).
  Future<({String code, VescXferStats? stats})> read(
      {void Function(double)? onProgress}) async {
    final r = await _ble.fileRequest(IpcCmd.lispRead, {}, onProgress: onProgress);
    return (code: r['code'] as String? ?? '', stats: _statsOf(r));
  }

  /// Erase + upload [code]; optionally start it running afterwards.
  ///
  /// [stopFirst] stops the running script before the erase — see
  /// [VescLink.uploadCode].
  Future<VescXferStats?> upload(String code,
      {bool run = false,
      bool stopFirst = false,
      void Function(double)? onProgress}) async {
    final r = await _ble.fileRequest(IpcCmd.lispUpload,
        {'code': code, 'run': run, 'stopFirst': stopFirst},
        onProgress: onProgress);
    return _statsOf(r);
  }

  VescXferStats? _statsOf(Map<String, dynamic> r) {
    final s = r['stats'];
    return s is Map ? VescXferStats.fromMap(Map<String, dynamic>.from(s)) : null;
  }

  Future<void> run() => _ble.fileRequest(IpcCmd.lispRun, {});
  Future<void> stop() => _ble.fileRequest(IpcCmd.lispStop, {});

  /// Begin / end periodic GET_STATS polling (drives [stats]).
  Future<void> startStats() => _ble.fileRequest(IpcCmd.lispStatsStart, {});
  Future<void> stopStats() => _ble.fileRequest(IpcCmd.lispStatsStop, {});

  /// One GET_STATS snapshot on demand, independent of the poller.
  Future<LispStats> statsOnce({bool all = true}) async {
    final r = await _ble.fileRequest(IpcCmd.lispStatsOnce, {'all': all});
    final s = r['stats'];
    return s is Map
        ? LispStats.fromMap(Map<String, dynamic>.from(s))
        : const LispStats(
            cpu: 0, heap: 0, mem: 0, stack: 0, doneCtx: '', bindings: []);
  }

  // ---- console (asynchronous script output) ----

  /// Live script output. Only flows between [subscribeConsole]`(true)` and
  /// `(false)`; the ring in the background isolate buffers regardless, so
  /// [readConsole] can catch up on anything missed.
  Stream<LispConsoleLine> get console => _ble.lispConsole;

  Future<LispConsoleChunk> readConsole(
      {int sinceSeq = 0, int maxLines = 200}) async {
    final r = await _ble.fileRequest(
        IpcCmd.lispConsoleRead, {'sinceSeq': sinceSeq, 'maxLines': maxLines});
    final c = r['chunk'];
    return c is Map
        ? LispConsoleChunk.fromMap(Map<String, dynamic>.from(c))
        : const LispConsoleChunk(
            lines: [], nextSeq: 0, dropped: 0, alive: false);
  }

  Future<void> clearConsole() => _ble.fileRequest(IpcCmd.lispConsoleClear, {});

  Future<void> subscribeConsole(bool on) =>
      _ble.fileRequest(IpcCmd.lispConsoleSub, {'on': on});

  /// Dump unrecognised packets into the console and return the per-command-id
  /// histogram — how the framing of an unknown reply gets worked out on real
  /// hardware.
  Future<Map<int, int>> setConsoleDebug(bool on) async {
    final r = await _ble.fileRequest(IpcCmd.lispConsoleDebug, {'on': on});
    final h = r['histogram'];
    if (h is! Map) return const {};
    return {
      for (final e in h.entries)
        int.tryParse('${e.key}') ?? -1: (e.value as num?)?.toInt() ?? 0
    };
  }

  // ---- target selection ----

  /// Talk to the VESC through the head unit's NUS bridge.
  Future<void> selectHeadUnit() =>
      _ble.fileRequest(IpcCmd.vescSelect, {'kind': VescTargetKind.headUnit.name});

  /// Connect to a stand-alone NUS adapter and talk to the VESC through it.
  Future<void> selectDirect(String remoteId, String name) =>
      _ble.fileRequest(IpcCmd.vescSelect, {
        'kind': VescTargetKind.direct.name,
        'remoteId': remoteId,
        'name': name,
      });

  /// Editor opened — bring the persisted target up.
  Future<void> resume() => _ble.fileRequest(IpcCmd.vescResume, {});

  /// Editor closed — let go of a direct adapter link.
  Future<void> release() => _ble.fileRequest(IpcCmd.vescRelease, {});
}
