/// BLE client for the VESC BLE Helper, living in the background isolate
/// alongside [BleService].
///
/// It runs here — not in the UI — for the same reason everything else BLE
/// does: flutter_blue_plus has one native side per process, and scanning or
/// connecting from a second FlutterEngine races the head-unit link. The
/// helper is simply a SECOND GATT connection from this isolate, exactly like
/// the LISP editor's direct VESC adapter, so both can be up at once.
///
/// Only the config service and OTA are here. The helper's NUS bridge is the
/// standard Nordic UART service, so `DirectNusTransport` already speaks to a
/// VESC through it — see helper_protocol.dart.
library;

import 'dart:async';
import 'dart:math';

import 'package:crypto/crypto.dart' show sha256;
import 'package:flutter/foundation.dart';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';

import 'helper_protocol.dart';

enum HelperConnState { idle, scanning, connecting, connected }

class HelperException implements Exception {
  const HelperException(this.message);
  final String message;
  @override
  String toString() => message;
}

/// Awaitable FIFO for OTA status round-trips.
class _EventQueue<T> {
  final _items = <T>[];
  final _waiters = <Completer<T>>[];

  void add(T value) {
    while (_waiters.isNotEmpty) {
      final w = _waiters.removeAt(0);
      if (w.isCompleted) continue; // timed out already
      w.complete(value);
      return;
    }
    _items.add(value);
  }

  Future<T> next(Duration timeout) {
    if (_items.isNotEmpty) return Future.value(_items.removeAt(0));
    final c = Completer<T>();
    _waiters.add(c);
    return c.future.timeout(timeout, onTimeout: () {
      _waiters.remove(c);
      throw TimeoutException('helper event', timeout);
    });
  }

  void clear() => _items.clear();
}

class HelperService {
  HelperService._();
  static final HelperService instance = HelperService._();

  // ---- outbound (to the UI, via ble_host) ----
  final _state = StreamController<HelperConnState>.broadcast();
  final _status = StreamController<HelperStatus>.broadcast();
  final _params = StreamController<PasParams>.broadcast();
  final _bindings = StreamController<ButtonBinding>.broadcast();
  final _scanHits = StreamController<ScanHit>.broadcast();
  final _log = StreamController<String>.broadcast();
  final _otaProgress = StreamController<double>.broadcast();

  Stream<HelperConnState> get stateChanges => _state.stream;
  Stream<HelperStatus> get statusUpdates => _status.stream;
  Stream<PasParams> get paramUpdates => _params.stream;
  Stream<ButtonBinding> get bindingUpdates => _bindings.stream;
  Stream<ScanHit> get scanHits => _scanHits.stream;
  Stream<String> get logLines => _log.stream;
  Stream<double> get otaProgress => _otaProgress.stream;

  HelperConnState _conn = HelperConnState.idle;
  HelperConnState get connState => _conn;
  bool get connected => _conn == HelperConnState.connected;

  /// Helper's running firmware version (DIS), null on older firmware.
  String? fwVersion;

  /// An OTA transfer is running; config writes are refused meanwhile.
  bool busy = false;

  BluetoothDevice? _device;
  StreamSubscription<BluetoothConnectionState>? _connSub;
  BluetoothCharacteristic? _cfgCtrl, _otaCtrl, _otaData;
  final _otaEvents = _EventQueue<({int st, int detail})>();

  int get _mtu => max(23, _device?.mtuNow ?? 23);

  void _emit(String line) {
    debugPrint('[helper] $line');
    if (!_log.isClosed) _log.add(line);
  }

  void _setState(HelperConnState s) {
    _conn = s;
    if (!_state.isClosed) _state.add(s);
  }

  // ---- connection ----

  /// Find the helper and connect. The device advertises the NUS service UUID
  /// and puts its name only in the scan response, so we filter by UUID and
  /// confirm the name.
  Future<void> connect({String? remoteId}) async {
    if (_conn == HelperConnState.connecting || connected) return;
    _setState(HelperConnState.connecting);
    try {
      final device = remoteId != null
          ? BluetoothDevice.fromId(remoteId)
          : await _find();
      if (device == null) {
        _emit('helper not found');
        _setState(HelperConnState.idle);
        return;
      }
      await device.connect(mtu: 512, timeout: const Duration(seconds: 20));
      _device = device;
      _connSub = device.connectionState.listen((s) {
        if (s == BluetoothConnectionState.disconnected) _onDisconnected();
      });
      await _discover(device);
      _setState(HelperConnState.connected);
      _emit('connected ${device.remoteId} (MTU $_mtu, fw ${fwVersion ?? '?'})');
      getParams();
    } catch (e) {
      _emit('connect failed: $e');
      await disconnect();
      rethrow;
    }
  }

  Future<void> disconnect() async {
    final d = _device;
    try {
      await d?.disconnect();
    } catch (_) {}
    _onDisconnected();
  }

  Future<BluetoothDevice?> _find() async {
    _setState(HelperConnState.scanning);
    final found = Completer<BluetoothDevice>();
    final sub = FlutterBluePlus.onScanResults.listen((results) {
      for (final r in results) {
        final named = r.advertisementData.advName == kHelperDeviceName ||
            r.device.platformName == kHelperDeviceName;
        if (named && !found.isCompleted) found.complete(r.device);
      }
    });
    try {
      await FlutterBluePlus.startScan(
        withServices: [Guid(kHelperNusServiceUuid)],
        timeout: const Duration(seconds: 10),
      );
      return await found.future.timeout(const Duration(seconds: 12));
    } catch (_) {
      return null;
    } finally {
      await sub.cancel();
      try {
        await FlutterBluePlus.stopScan();
      } catch (_) {}
    }
  }

  Future<void> _discover(BluetoothDevice device) async {
    final chars = <Guid, BluetoothCharacteristic>{};
    for (final svc in await device.discoverServices()) {
      for (final c in svc.characteristics) {
        chars[c.uuid] = c;
      }
    }
    BluetoothCharacteristic need(String uuid) {
      final c = chars[Guid(uuid)];
      if (c == null) throw HelperException('missing characteristic $uuid');
      return c;
    }

    _cfgCtrl = need(kCfgCtrlUuid);
    _otaCtrl = need(kHelperOtaCtrlUuid);
    _otaData = need(kHelperOtaDataUuid);

    Future<void> subscribe(
        BluetoothCharacteristic c, void Function(Uint8List) handler) async {
      final sub = c.onValueReceived
          .listen((v) => handler(Uint8List.fromList(v)));
      device.cancelWhenDisconnected(sub);
      await c.setNotifyValue(true);
    }

    await subscribe(_cfgCtrl!, _onCtrl);
    await subscribe(need(kCfgStatusUuid), _onStatus);
    await subscribe(need(kCfgScanUuid), _onScan);
    await subscribe(_otaCtrl!, _onOta);

    final rev = chars[Guid(kFwRevisionUuid)];
    if (rev != null) {
      try {
        fwVersion = String.fromCharCodes(await rev.read()).trim();
      } catch (_) {
        // Older firmware simply doesn't have it.
      }
    }
  }

  void _onDisconnected() {
    if (_device == null && _conn == HelperConnState.idle) return;
    _device = null;
    _connSub?.cancel();
    _connSub = null;
    _cfgCtrl = _otaCtrl = _otaData = null;
    fwVersion = null;
    busy = false;
    _setState(HelperConnState.idle);
    _emit('disconnected');
  }

  // ---- notifications ----

  void _onCtrl(Uint8List data) {
    if (data.isEmpty) return;
    final rsp = data[0];
    if (rsp == 0x85 && data.length >= 1 + PasParams.length) {
      final p = PasParams.decode(data.sublist(1, 1 + PasParams.length));
      if (p == null) {
        _emit('unsupported params version ${data[1]} — update the helper');
        return;
      }
      if (!_params.isClosed) _params.add(p);
      // Bindings follow the params so the UI can paint the whole page at once.
      for (var i = 0; i < kBtnUiSlots; i++) {
        getBinding(i);
      }
    } else if (rsp == 0x89 && data.length >= 16) {
      final b = ButtonBinding.decode(data);
      if (b != null && !_bindings.isClosed) _bindings.add(b);
    } else {
      final cmd = rsp & 0x7F;
      final ok = data.length >= 2 && data[1] == 0;
      _emit('command $cmd: ${ok ? 'ok' : 'error'}');
      // Anything that changes state on the helper: re-read, don't assume.
      if (cmd == HelperCmd.bindButton ||
          cmd == HelperCmd.bindCadence ||
          cmd == HelperCmd.unbind ||
          cmd == HelperCmd.setParams) {
        getParams();
      }
    }
  }

  void _onStatus(Uint8List data) {
    final s = HelperStatus.decode(data);
    if (s != null && !_status.isClosed) _status.add(s);
  }

  void _onScan(Uint8List data) {
    final hit = ScanHit.decode(data);
    if (hit != null && !_scanHits.isClosed) _scanHits.add(hit);
  }

  void _onOta(Uint8List data) {
    if (data.length < 5) return;
    _otaEvents.add((
      st: data[0],
      detail: ByteData.sublistView(data).getUint32(1, Endian.little),
    ));
  }

  // ---- config commands ----

  Future<void> _ctrl(List<int> payload) async {
    final c = _cfgCtrl;
    if (c == null) throw const HelperException('not connected');
    await c.write(payload); // with response, like the reference GUI
  }

  /// Ask the HELPER to scan for gadgets. It hunts for sleepy peripherals the
  /// phone may never see, and takes about 6 s.
  Future<void> scanRemotes(int what) => _ctrl([HelperCmd.scan, what]);

  Future<void> unbind(int what) => _ctrl([HelperCmd.unbind, what]);
  Future<void> getParams() => _ctrl([HelperCmd.getParams]);
  Future<void> setParams(Uint8List blob) =>
      _ctrl([HelperCmd.setParams, ...blob]);
  Future<void> setThrottle(int v) => _ctrl([HelperCmd.setThrottle, v]);
  Future<void> getBinding(int idx) => _ctrl([HelperCmd.getBinding, idx]);
  Future<void> setBinding(ButtonBinding b) => _ctrl(b.encodeSet());

  Future<void> bind(ScanHit hit) => _ctrl([
        hit.what == kWhatButton ? HelperCmd.bindButton : HelperCmd.bindCadence,
        hit.addrType,
        ...hit.addr,
      ]);

  // ---- OTA ----

  /// Flash [image] to the helper. Progress is reported 0..1; the device
  /// reboots on success, so the link drops right after.
  Future<void> flashFirmware(Uint8List image) async {
    final ctrl = _otaCtrl;
    final data = _otaData;
    if (ctrl == null || data == null) {
      throw const HelperException('not connected');
    }
    if (busy) throw const HelperException('a transfer is already running');
    busy = true;
    try {
      final sha = sha256.convert(image).bytes;
      _emit('ota: ${image.length} bytes, erasing partition…');
      _otaEvents.clear();

      final begin = Uint8List(1 + 4 + 32);
      begin[0] = HelperOta.opBegin;
      ByteData.sublistView(begin).setUint32(1, image.length, Endian.little);
      begin.setRange(5, 37, sha);
      await ctrl.write(begin);

      // The erase blocks the helper for seconds; this ack only comes after.
      final ready = await _otaEvents.next(const Duration(seconds: 30));
      if (ready.st != HelperOta.stReady) {
        throw HelperException(
            'begin rejected: 0x${ready.st.toRadixString(16)}/${ready.detail}');
      }

      _emit('ota: transferring…');
      final chunk = min(244, _mtu - 3);
      for (var off = 0; off < image.length; off += chunk) {
        await data.write(image.sublist(off, min(off + chunk, image.length)),
            withoutResponse: true);
        if (off % (32 * chunk) == 0) _otaProgress.add(off / image.length);
      }
      _otaProgress.add(1);

      await ctrl.write([HelperOta.opEnd]);
      while (true) {
        final ev = await _otaEvents.next(const Duration(seconds: 30));
        if (ev.st == HelperOta.stProgress) continue;
        if (ev.st == HelperOta.stDone) {
          _emit('ota done — helper is rebooting');
          return;
        }
        throw HelperException(
            'ota error: 0x${ev.st.toRadixString(16)}/${ev.detail}');
      }
    } catch (e) {
      _emit('ota: $e');
      try {
        await _otaCtrl?.write([HelperOta.opAbort]);
      } catch (_) {}
      rethrow;
    } finally {
      busy = false;
    }
  }
}
