/// Foreground BLE connection wrapper around flutter_blue_plus.
///
/// Holds at most one peripheral connection. Owns the chunk encoder/decoder
/// and exposes high-level send / command-stream APIs.
library;

import 'dart:async';
import 'dart:typed_data';

import 'package:flutter_blue_plus/flutter_blue_plus.dart';

import 'messages.dart';
import 'protocol.dart';
import 'uuids.dart';

enum BleConnState { idle, scanning, connecting, connected, disconnected }

class BleService {
  BleService._();
  static final BleService instance = BleService._();

  final _stateCtrl = StreamController<BleConnState>.broadcast();
  final _cmdCtrl = StreamController<InboundCommand>.broadcast();
  final _encoder = ChunkedEncoder();
  final _decoder = ChunkedDecoder();

  BleConnState _state = BleConnState.idle;
  BluetoothDevice? _device;
  BluetoothCharacteristic? _inbound;
  BluetoothCharacteristic? _outbound;
  StreamSubscription<List<int>>? _outSub;
  StreamSubscription<BluetoothConnectionState>? _connSub;

  Stream<BleConnState> get state => _stateCtrl.stream;
  Stream<InboundCommand> get commands => _cmdCtrl.stream;
  BleConnState get currentState => _state;
  BluetoothDevice? get device => _device;

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

  Future<void> connect(BluetoothDevice device) async {
    await disconnect();
    _device = device;
    _setState(BleConnState.connecting);

    _connSub = device.connectionState.listen((s) {
      if (s == BluetoothConnectionState.disconnected) {
        _setState(BleConnState.disconnected);
      }
    });

    await device.connect(autoConnect: false, mtu: 247);
    // Some Android stacks ignore the connect mtu param — request explicitly.
    try {
      await device.requestMtu(247);
    } catch (_) {}

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
    _outSub = _outbound!.onValueReceived.listen(_onOutbound);

    _setState(BleConnState.connected);
  }

  Future<void> disconnect() async {
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
