/// Foreground BLE connection wrapper around flutter_blue_plus.
///
/// Holds at most one peripheral connection, persists the last paired
/// device's remote id, and auto-reconnects whenever the link drops or the
/// app is relaunched. Owns the chunk encoder/decoder and exposes
/// high-level send / command-stream APIs.
library;

import 'dart:async';
import 'dart:typed_data';

import 'package:flutter_blue_plus/flutter_blue_plus.dart';
import 'package:shared_preferences/shared_preferences.dart';

import '../bridge/foreground_bridge.dart';
import 'messages.dart';
import 'protocol.dart';
import 'uuids.dart';

enum BleConnState { idle, scanning, connecting, connected, disconnected }

class BleService {
  BleService._();
  static final BleService instance = BleService._();

  static const _prefSavedId = 'ble_saved_remote_id_v1';

  final _stateCtrl = StreamController<BleConnState>.broadcast();
  final _cmdCtrl = StreamController<InboundCommand>.broadcast();
  final _encoder = ChunkedEncoder();
  final _decoder = ChunkedDecoder();
  final _fg = ForegroundBridge();

  BleConnState _state = BleConnState.idle;
  BluetoothDevice? _device;
  BluetoothCharacteristic? _inbound;
  BluetoothCharacteristic? _outbound;
  StreamSubscription<List<int>>? _outSub;
  StreamSubscription<BluetoothConnectionState>? _connSub;

  // Tracks the device we're actively auto-reconnecting to. Cleared by
  // an explicit user-initiated disconnect (e.g. forget device) so we
  // don't fight the user's intent.
  String? _savedRemoteId;
  bool _userInitiatedDisconnect = false;

  Stream<BleConnState> get state => _stateCtrl.stream;
  Stream<InboundCommand> get commands => _cmdCtrl.stream;
  BleConnState get currentState => _state;
  BluetoothDevice? get device => _device;
  String? get savedRemoteId => _savedRemoteId;

  /// Pull the persisted remote id and, if present, kick off a background
  /// auto-connect attempt. Safe to call once at app boot.
  Future<void> resumeIfPaired() async {
    final p = await SharedPreferences.getInstance();
    _savedRemoteId = p.getString(_prefSavedId);
    if (_savedRemoteId == null) return;
    // Use `unawaited` semantics — boot path mustn't block on a BLE scan.
    // flutter_blue_plus's autoConnect:true returns immediately and lets
    // the OS hold the connection request.
    _attachKnownDevice(_savedRemoteId!);
  }

  void _attachKnownDevice(String remoteId) {
    final dev = BluetoothDevice.fromId(remoteId);
    _device = dev;
    _userInitiatedDisconnect = false;
    _connSub?.cancel();
    _connSub = dev.connectionState.listen(_onConnectionStateChanged);
    _setState(BleConnState.connecting);
    // autoConnect:true → Android queues the GATT connect; the OS wakes
    // when the peripheral advertises. iOS handles the equivalent through
    // CoreBluetooth's stored-peripheral mechanism, but only if the user
    // accepted the system pairing prompt on first connect.
    dev.connect(autoConnect: true).catchError((_) {});
  }

  /// The head unit's adv packet carries the NUS UUID (legacy, for VESC
  /// Tool) plus the device name in the scan response. The NotifBridge
  /// service isn't advertised because two 128-bit UUIDs don't fit in
  /// 31 bytes — so we scan unfiltered and let the user pick by name.
  Future<List<ScanResult>> scan({Duration timeout = const Duration(seconds: 6)}) async {
    _setState(BleConnState.scanning);
    final results = <String, ScanResult>{};
    final sub = FlutterBluePlus.scanResults.listen((batch) {
      for (final r in batch) {
        final name = r.advertisementData.advName.isNotEmpty
            ? r.advertisementData.advName
            : r.device.platformName;
        if (name.isEmpty) continue;
        results[r.device.remoteId.str] = r;
      }
    });
    await FlutterBluePlus.startScan(timeout: timeout);
    await FlutterBluePlus.isScanning.where((s) => !s).first;
    await sub.cancel();
    _setState(BleConnState.idle);
    return results.values.toList();
  }

  /// One-shot pairing flow triggered from the UI. Persists the remote id
  /// on first successful service discovery so subsequent app launches
  /// auto-resume without going through the scanner.
  Future<void> connect(BluetoothDevice device) async {
    await _teardown();
    _device = device;
    _userInitiatedDisconnect = false;
    _setState(BleConnState.connecting);

    _connSub = device.connectionState.listen(_onConnectionStateChanged);

    await device.connect(autoConnect: true, mtu: 247);
    // Some Android stacks ignore the connect mtu param — request explicitly.
    try {
      await device.requestMtu(247);
    } catch (_) {}

    await _completeHandshake(device);

    // Persist only after a clean handshake so we don't trap ourselves
    // auto-reconnecting to a device that isn't actually a NotifBridge
    // head unit.
    final p = await SharedPreferences.getInstance();
    await p.setString(_prefSavedId, device.remoteId.str);
    _savedRemoteId = device.remoteId.str;

    await _fg.start();
  }

  Future<void> _completeHandshake(BluetoothDevice device) async {
    final services = await device.discoverServices();
    final svc = services.firstWhere(
      (s) => s.uuid.toString().toLowerCase() == NotifBridgeUuids.service,
      orElse: () => throw StateError('NotifBridge service not found'),
    );
    _inbound = svc.characteristics.firstWhere(
      (c) => c.uuid.toString().toLowerCase() == NotifBridgeUuids.charInbound,
    );
    _outbound = svc.characteristics.firstWhere(
      (c) => c.uuid.toString().toLowerCase() == NotifBridgeUuids.charOutbound,
    );
    await _outbound!.setNotifyValue(true);
    await _outSub?.cancel();
    _outSub = _outbound!.onValueReceived.listen(_onOutbound);
    _setState(BleConnState.connected);
  }

  void _onConnectionStateChanged(BluetoothConnectionState s) {
    if (s == BluetoothConnectionState.connected && _device != null) {
      // OS-driven (auto)reconnect just brought the link back up — redo
      // service discovery so handles are fresh and notifications resume.
      _completeHandshake(_device!).then((_) => _fg.start()).catchError((e) {
        _setState(BleConnState.disconnected);
      });
    } else if (s == BluetoothConnectionState.disconnected) {
      _setState(BleConnState.disconnected);
      // autoConnect:true keeps the OS-level reconnect queue active by
      // itself; nothing to retry here. If the user explicitly disconnected
      // (forget device) we stop the foreground service so the persistent
      // notification goes away.
      if (_userInitiatedDisconnect) {
        _fg.stop();
      }
    }
  }

  /// User-initiated "forget device" — drops the persisted remote id and
  /// tears the link down without re-arming auto-reconnect.
  Future<void> forget() async {
    _userInitiatedDisconnect = true;
    final p = await SharedPreferences.getInstance();
    await p.remove(_prefSavedId);
    _savedRemoteId = null;
    await _teardown();
    await _fg.stop();
  }

  Future<void> _teardown() async {
    await _connSub?.cancel();
    await _outSub?.cancel();
    _connSub = null;
    _outSub = null;
    try {
      await _device?.disconnect();
    } catch (_) {}
    _device = null;
    _inbound = null;
    _outbound = null;
    _setState(BleConnState.idle);
  }

  Future<void> sendNotification(NotificationMsg n) =>
      _send(PduType.notification, n.encode());

  Future<void> sendMedia(MediaMsg m) => _send(PduType.media, m.encode());

  Future<void> sendIcon(IconMsg i) => _send(PduType.icon, i.encode());

  Future<void> sendAlbumArt(IconMsg i) => _send(PduType.albumArt, i.encode());

  Future<void> _send(PduType type, Uint8List body) async {
    final ch = _inbound;
    if (ch == null) return;
    final chunks = _encoder.encode(type, body);
    for (final c in chunks) {
      await ch.write(c, withoutResponse: true);
    }
  }

  void _onOutbound(List<int> raw) {
    final out = _decoder.feed(Uint8List.fromList(raw));
    if (out == null) return;
    if (out.type == PduType.command) {
      final cmd = InboundCommand.parse(out.body);
      if (cmd != null) _cmdCtrl.add(cmd);
    }
  }

  void _setState(BleConnState s) {
    _state = s;
    _stateCtrl.add(s);
  }
}
