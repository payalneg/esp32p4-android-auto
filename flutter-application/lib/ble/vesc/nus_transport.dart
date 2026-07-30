/// Nordic UART Service byte pipes for the VESC protocol layer.
///
/// [VescLink] used to reach straight into `BleService` for the head unit's NUS
/// characteristics, which tied LISP editing to a connected head unit. It now
/// talks to a [NusTransport] instead, so the same protocol code drives either:
///
///   * [HeadUnitNusTransport] — the head unit's built-in bridge (main/ble_nus.c),
///     which relays the payload to the VESC over CAN, or
///   * [DirectNusTransport] — a stand-alone VESC BLE adapter (VESC Express, a
///     VESC 6's built-in BLE, an HM-10-class dongle) that the app connects to as
///     a *second*, independent GATT link. Works with no head unit at all.
///
/// The bytes on the wire are identical either way — plain VESC packets — because
/// the head unit's bridge is transparent (it re-frames but does not wrap).
///
/// Everything here lives in the background BLE isolate; `BluetoothDevice` /
/// `BluetoothCharacteristic` never cross the port to the UI.
library;

import 'dart:async';
import 'dart:typed_data';

import 'package:flutter/foundation.dart' show debugPrint;
import 'package:flutter/services.dart' show PlatformException;
import 'package:flutter_blue_plus/flutter_blue_plus.dart';

import '../ble_service.dart';
import '../lisp_models.dart';
import '../uuids.dart';
import 'vesc_errors.dart';

abstract class NusTransport {
  /// Identity of the current physical binding. [VescLink] re-subscribes and
  /// resets its packet decoder whenever this changes (reconnect → fresh
  /// characteristic objects → stale half-parsed bytes must be dropped).
  Object get bindKey;

  /// Negotiated ATT MTU of the link (23 until the exchange completes).
  int get mtu;

  /// Raw NUS TX notifications. Only valid after [ensureReady].
  Stream<List<int>> get inbound;

  /// Bring the link up (connect if needed) and subscribe to NUS TX.
  /// Throws [VescLispException] with an i18n key when it can't.
  Future<void> ensureReady();

  /// Write one already-framed VESC packet, chunked to the link's MTU.
  Future<void> write(Uint8List framed);

  /// Ask for a faster connection interval for the duration of a transfer.
  /// Best-effort — a no-op wherever the platform/adapter won't do it.
  Future<void> setFast(bool fast);

  /// Short label for logs.
  String get label;
}

/// Chunked write shared by both transports.
///
/// Caps chunks at 244 B even when the MTU allows more: that is the size the
/// head-unit path has been running with, and some adapters advertise a large
/// MTU while their NUS RX attribute stays small. Requests are tiny anyway — the
/// throughput that matters is on the notification (reply) side.
Future<void> writeChunked(
    BluetoothCharacteristic rx, Uint8List packet, int mtu) async {
  final chunkSize = (mtu - 3).clamp(20, 244);
  var off = 0;
  while (off < packet.length) {
    final end =
        (off + chunkSize) < packet.length ? off + chunkSize : packet.length;
    await _writeChunk(rx, Uint8List.sublistView(packet, off, end));
    off = end;
  }
}

/// flutter_blue_plus's `withoutResponse` path doesn't queue against the GATT
/// stack, so back-to-back chunks can hit ERROR_GATT_WRITE_REQUEST_BUSY (201).
/// Retry with a short backoff, falling back to acked writes.
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

/// The head unit's NUS bridge — the original path, now behind the interface.
class HeadUnitNusTransport implements NusTransport {
  HeadUnitNusTransport._();
  static final HeadUnitNusTransport instance = HeadUnitNusTransport._();

  BluetoothCharacteristic? _notifyOn;

  bool get available =>
      BleService.instance.isConnected && BleService.instance.supportsLisp;

  @override
  Object get bindKey => BleService.instance.nusTxChar ?? this;

  @override
  int get mtu => BleService.instance.negotiatedMtu;

  @override
  Stream<List<int>> get inbound =>
      BleService.instance.nusTxChar?.onValueReceived ?? const Stream.empty();

  @override
  String get label => 'head-unit';

  @override
  Future<void> ensureReady() async {
    final tx = BleService.instance.nusTxChar;
    final rx = BleService.instance.nusRxChar;
    if (tx == null || rx == null || !BleService.instance.isConnected) {
      throw const VescLispException('lisp.err.noconn');
    }
    // Re-subscribing on every op would be a GATT descriptor write each time;
    // the characteristic object is recreated on re-discovery, so identity is
    // the right "do I still hold this subscription" test.
    if (!identical(_notifyOn, tx)) {
      await tx.setNotifyValue(true);
      _notifyOn = tx;
    }
  }

  @override
  Future<void> write(Uint8List framed) async {
    final rx = BleService.instance.nusRxChar;
    if (rx == null) throw const VescLispException('lisp.err.noconn');
    await writeChunked(rx, framed, mtu);
  }

  @override
  Future<void> setFast(bool fast) => BleService.instance.setLinkSpeed(fast: fast);
}

/// A stand-alone NUS adapter the app connects to itself.
///
/// Independent of [BleService]'s single head-unit link: both can be up at the
/// same time (the phone keeps pushing notifications/media to the head unit
/// while the LISP editor talks to the adapter).
///
/// Lifetime is owned by [VescTarget]: created on selection, disposed when the
/// editor closes. While selected, an unexpected drop is retried with a backoff.
class DirectNusTransport implements NusTransport {
  DirectNusTransport({required this.remoteId, required this.name});

  final String remoteId;
  final String name;

  BluetoothDevice? _dev;
  BluetoothCharacteristic? _rx;
  BluetoothCharacteristic? _tx;
  StreamSubscription<BluetoothConnectionState>? _connSub;
  Timer? _retryTimer;
  int _retryAttempt = 0;
  bool _wanted = false;
  bool _connecting = false;

  VescLinkState _state = VescLinkState.idle;
  final _stateCtrl = StreamController<VescLinkState>.broadcast();

  VescLinkState get state => _state;
  Stream<VescLinkState> get states => _stateCtrl.stream;

  @override
  Object get bindKey => _tx ?? this;

  @override
  int get mtu => _dev?.mtuNow ?? 23;

  @override
  Stream<List<int>> get inbound =>
      _tx?.onValueReceived ?? const Stream.empty();

  @override
  String get label => 'direct:$name';

  void _setState(VescLinkState s) {
    if (_state == s) return;
    _state = s;
    _stateCtrl.add(s);
  }

  /// Connect + discover. Safe to call repeatedly; returns once the NUS
  /// characteristics are live.
  Future<void> connect() async {
    _wanted = true;
    if (_state == VescLinkState.connected && _rx != null && _tx != null) return;
    if (_connecting) {
      // A retry is already in flight — wait for it to settle instead of
      // starting a second GATT connect on the same device.
      await states
          .firstWhere((s) => s != VescLinkState.connecting)
          .timeout(const Duration(seconds: 25), onTimeout: () => _state);
      if (_state == VescLinkState.connected) return;
      throw const VescLispException('lisp.err.connect');
    }
    _connecting = true;
    _setState(VescLinkState.connecting);
    try {
      final dev = BluetoothDevice.fromId(remoteId);
      _dev = dev;
      await _connSub?.cancel();
      _connSub = dev.connectionState.listen(_onConnState);
      if (!dev.isConnected) {
        // autoConnect:false → a real, prompt connect attempt (this is a
        // foreground, user-initiated session; we do our own retries).
        // mtu must be null when we request it ourselves below.
        await dev
            .connect(autoConnect: false, mtu: null)
            .timeout(const Duration(seconds: 20));
      }
      // Bigger MTU = the adapter can push a whole code chunk in one
      // notification instead of three. Best-effort: small adapters stay at 23.
      try {
        await dev.requestMtu(512);
      } catch (_) {}
      await _discover(dev);
      _retryAttempt = 0;
      _setState(VescLinkState.connected);
    } on VescLispException {
      _setState(VescLinkState.failed);
      rethrow;
    } catch (e) {
      debugPrint('[vesc] direct connect failed: $e');
      _setState(VescLinkState.failed);
      throw const VescLispException('lisp.err.connect');
    } finally {
      _connecting = false;
    }
  }

  Future<void> _discover(BluetoothDevice dev) async {
    final services = await dev.discoverServices();
    BluetoothCharacteristic? rx;
    BluetoothCharacteristic? tx;
    for (final s in services) {
      if (s.uuid.toString().toLowerCase() != NusUuids.service) continue;
      for (final c in s.characteristics) {
        final u = c.uuid.toString().toLowerCase();
        if (u == NusUuids.rx) rx = c;
        if (u == NusUuids.tx) tx = c;
      }
    }
    if (rx == null || tx == null) {
      // Not a NUS adapter — don't sit on the link.
      try {
        await dev.disconnect();
      } catch (_) {}
      throw const VescLispException('lisp.err.nonus');
    }
    await tx.setNotifyValue(true);
    _rx = rx;
    _tx = tx;
  }

  void _onConnState(BluetoothConnectionState s) {
    if (s != BluetoothConnectionState.disconnected) return;
    _rx = null;
    _tx = null;
    if (!_wanted) {
      _setState(VescLinkState.idle);
      return;
    }
    _setState(VescLinkState.failed);
    _scheduleRetry();
  }

  void _scheduleRetry() {
    _retryTimer?.cancel();
    _retryAttempt++;
    final delayMs = (500 * _retryAttempt).clamp(500, 4000);
    _retryTimer = Timer(Duration(milliseconds: delayMs), () async {
      if (!_wanted) return;
      try {
        await connect();
      } catch (_) {
        if (_wanted) _scheduleRetry();
      }
    });
  }

  @override
  Future<void> ensureReady() async {
    if (_rx == null || _tx == null || !(_dev?.isConnected ?? false)) {
      await connect();
    }
    if (_rx == null || _tx == null) {
      throw const VescLispException('lisp.err.noconn');
    }
  }

  @override
  Future<void> write(Uint8List framed) async {
    final rx = _rx;
    if (rx == null) throw const VescLispException('lisp.err.noconn');
    await writeChunked(rx, framed, mtu);
  }

  @override
  Future<void> setFast(bool fast) async {
    final dev = _dev;
    if (dev == null || !dev.isConnected) return;
    try {
      await dev.requestConnectionPriority(
          connectionPriorityRequest:
              fast ? ConnectionPriority.high : ConnectionPriority.balanced);
    } catch (_) {
      // android-only API, or the adapter refused — speed is a hint anyway
    }
  }

  /// Drop the link and stop retrying (editor closed / another target picked).
  Future<void> dispose() async {
    _wanted = false;
    _retryTimer?.cancel();
    _retryTimer = null;
    await _connSub?.cancel();
    _connSub = null;
    final dev = _dev;
    _rx = null;
    _tx = null;
    if (dev != null) {
      try {
        await dev.disconnect();
      } catch (_) {}
    }
    _dev = null;
    _setState(VescLinkState.idle);
    await _stateCtrl.close();
  }
}
