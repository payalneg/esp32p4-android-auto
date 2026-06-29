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
import 'messages.dart';

export 'ble_service.dart' show BleConnState;

/// A scan hit, stripped to the JSON-safe fields the pairing UI needs (the
/// flutter_blue_plus ScanResult/BluetoothDevice objects can't cross isolates).
class ScanDevice {
  final String remoteId;
  final String name;
  final int rssi;
  const ScanDevice(this.remoteId, this.name, this.rssi);
}

/// Error surfaced from the task isolate over the port.
class IpcException implements Exception {
  final String kind;
  final String? msg;
  IpcException(this.kind, this.msg);
  @override
  String toString() => 'IpcException($kind, $msg)';
}

class BleProxy {
  BleProxy._();
  static final BleProxy instance = BleProxy._();

  // Mirror of BleService._prefSavedId — read directly so the UI can show the
  // saved device at first paint, before the task pushes its first state.
  static const _prefSavedId = 'ble_saved_remote_id_v1';

  final _stateCtrl = StreamController<BleConnState>.broadcast();
  BleConnState _state = BleConnState.idle;
  String? _savedRemoteId;
  bool _supportsFm = false;
  bool _supportsOta = false;
  bool _supportsBleOta = false;
  int _mtu = 247;

  int _nextId = 1;
  final _pending = <int, Completer<Map<String, dynamic>>>{};
  final _onProgress = <int, void Function(double)>{};
  final _onOtaUpload = <int, void Function(double)>{};
  final _onOtaVerify = <int, void Function()>{};
  bool _wired = false;

  Stream<BleConnState> get state => _stateCtrl.stream;
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
    }
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
    return IpcException(m['kind'] as String? ?? 'generic', m['msg'] as String?);
  }

  void _applyState(Map<String, dynamic> m) {
    _savedRemoteId = m['savedRemoteId'] as String?;
    _supportsFm = m['supportsFm'] as bool? ?? false;
    _supportsOta = m['supportsOta'] as bool? ?? false;
    _supportsBleOta = m['supportsBleOta'] as bool? ?? false;
    _mtu = (m['mtu'] as num?)?.toInt() ?? 247;
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

  Future<List<ScanDevice>> scan() async {
    final r = await _request(IpcCmd.scan);
    final list = (r['devices'] as List?) ?? const [];
    return [
      for (final d in list)
        ScanDevice(d['remoteId'] as String, d['name'] as String? ?? '',
            (d['rssi'] as num?)?.toInt() ?? 0)
    ];
  }

  Future<void> connect(String remoteId) =>
      _request(IpcCmd.connect, {'remoteId': remoteId});

  Future<void> forget() => _request(IpcCmd.forget);

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
