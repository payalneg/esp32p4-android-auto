/// Foreground BLE connection wrapper around flutter_blue_plus.
///
/// Holds at most one peripheral connection, persists the last paired
/// device's remote id, and auto-reconnects whenever the link drops or the
/// app is relaunched. Owns the chunk encoder/decoder and exposes
/// high-level send / command-stream APIs.
library;

import 'dart:async';
import 'dart:io' show Platform;
import 'dart:typed_data';

import 'package:flutter/services.dart' show PlatformException;
import 'package:flutter_blue_plus/flutter_blue_plus.dart';
import 'package:shared_preferences/shared_preferences.dart';

import '../bridge/foreground_bridge.dart';
import '../firmware/ble_ota.dart';
import '../firmware/ota_info.dart';
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
  // Optional — null when the head unit's firmware predates the time
  // characteristic. We only push the clock when this is non-null.
  BluetoothCharacteristic? _time;
  // Optional — null when the firmware predates the OTA-from-app feature.
  // Read on demand from the firmware-update screen.
  BluetoothCharacteristic? _otaInfo;
  // Optional — null when the firmware predates BLE OTA (...0007/...0008).
  // Present together; drive the over-BLE firmware flash (see bleOta).
  BluetoothCharacteristic? _otaCtrl;
  BluetoothCharacteristic? _otaData;
  // Optional — null when the firmware predates the file manager (...0009/...000a).
  // Present together; drive the device file browser (see FileManager).
  BluetoothCharacteristic? _fileCtrl;
  BluetoothCharacteristic? _fileData;
  // Optional — the head unit's separate Nordic UART Service (VESC-Tool bridge).
  // Present together; drive the LISP editor (see lib/ble/vesc/). Absent on
  // firmware without the bridge.
  BluetoothCharacteristic? _nusRx;
  BluetoothCharacteristic? _nusTx;
  Timer? _clockTimer;
  StreamSubscription<List<int>>? _outSub;
  StreamSubscription<BluetoothConnectionState>? _connSub;

  // Tracks the device we're actively auto-reconnecting to. Cleared by
  // an explicit user-initiated disconnect (e.g. forget device) so we
  // don't fight the user's intent.
  String? _savedRemoteId;
  bool _userInitiatedDisconnect = false;

  // Reconnect state machine. The OS-level autoConnect link can come up
  // (ACL connected) without the GATT handshake succeeding — and once it's
  // up, the stack will NOT re-emit `connected`, so a failed handshake
  // would wedge us in `disconnected` forever while the radio link stays
  // alive (the "reconnects but no data" symptom). We retry the handshake
  // and, if it still won't complete, force a clean disconnect+reconnect.
  bool _handshaking = false;
  int _reconnectAttempt = 0;
  Timer? _reconnectTimer;

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
    // mtu MUST be null with autoConnect — the package's default is 512
    // and the assertion `(mtu == null) || !autoConnect` would trip.
    dev.connect(autoConnect: true, mtu: null).catchError((_) {});
  }

  /// The head unit's adv packet carries the NUS UUID (legacy, for VESC
  /// Tool) plus the device name in the scan response. The NotifBridge
  /// service isn't advertised because two 128-bit UUIDs don't fit in
  /// 31 bytes — so we scan unfiltered and let the user pick by name.
  ///
  /// [mutateState] drives the scanning/idle state pushes. The pairing screen
  /// wants them; the LISP editor's adapter picker does not — it scans while a
  /// head unit may be connected, and clobbering the state would make the UI
  /// paint "not connected" and hide the head-unit tiles.
  Future<List<ScanResult>> scan(
      {Duration timeout = const Duration(seconds: 6),
      bool mutateState = true}) async {
    if (mutateState) _setState(BleConnState.scanning);
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
    if (mutateState) _setState(BleConnState.idle);
    return results.values.toList();
  }

  Completer<void>? _firstHandshake;

  /// One-shot pairing flow triggered from the UI. Persists the remote id
  /// only after the first clean handshake so we don't trap ourselves
  /// auto-reconnecting to a device that isn't actually a NotifBridge
  /// head unit.
  Future<void> connect(BluetoothDevice device) async {
    await _teardown();
    _device = device;
    _userInitiatedDisconnect = false;
    _setState(BleConnState.connecting);

    _firstHandshake = Completer<void>();
    _connSub = device.connectionState.listen(_onConnectionStateChanged);

    // flutter_blue_plus's `connect()` defaults mtu to 512, which conflicts
    // with autoConnect:true (the OS-level autoConnect path doesn't accept
    // a pre-negotiation MTU). Pass mtu:null explicitly and run requestMtu
    // ourselves once the link is up — see _completeHandshake().
    // With autoConnect:true this returns immediately; the link comes up
    // asynchronously through the connectionState stream.
    await device.connect(autoConnect: true, mtu: null);

    // Wait for connectionState→connected→_completeHandshake to succeed.
    // First-attempt timeout: 20 s covers a slow GATT discovery on real
    // hardware. If the device never advertises within that window, surface
    // the failure to the UI so the user can retry.
    try {
      await _firstHandshake!.future.timeout(const Duration(seconds: 20));
    } on TimeoutException {
      _firstHandshake = null;
      await _teardown();
      throw StateError('timeout: device did not connect within 20 s');
    }
    _firstHandshake = null;

    final p = await SharedPreferences.getInstance();
    await p.setString(_prefSavedId, device.remoteId.str);
    _savedRemoteId = device.remoteId.str;

    await _fg.start();
  }

  /// Connect by remote id. Used by the background task isolate, which receives
  /// only the serializable id over the port (not the BluetoothDevice object).
  Future<void> connectById(String remoteId) =>
      connect(BluetoothDevice.fromId(remoteId));

  /// Send a pre-encoded PDU body for the given kind. The background task gets
  /// the already-`encode()`d message bytes over the port and forwards them
  /// here, so this just maps the kind back to its [PduType].
  Future<void> sendRaw(String kind, Uint8List body) {
    switch (kind) {
      case 'notif':
        return _send(PduType.notification, body);
      case 'media':
        return _send(PduType.media, body);
      case 'icon':
        return _send(PduType.icon, body);
      case 'albumArt':
        return _send(PduType.albumArt, body);
    }
    return Future.value();
  }

  Future<void> _completeHandshake(BluetoothDevice device) async {
    try {
      // 512 matches the head unit's preferred MTU: a whole LISP read chunk
      // (~415 B framed) then arrives in one notification instead of two.
      // Android caps at 517 and negotiates down for peers that ask for less,
      // so this is safe against older firmware still preferring 256.
      await device.requestMtu(512);
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
    // Optional characteristics — absent on older firmware. Probe by UUID
    // and leave the field null if the head unit doesn't advertise it.
    _time = null;
    _otaInfo = null;
    _otaCtrl = null;
    _otaData = null;
    _fileCtrl = null;
    _fileData = null;
    for (final c in svc.characteristics) {
      final u = c.uuid.toString().toLowerCase();
      if (u == NotifBridgeUuids.charTime) _time = c;
      if (u == NotifBridgeUuids.charOtaInfo) _otaInfo = c;
      if (u == NotifBridgeUuids.charOtaCtrl) _otaCtrl = c;
      if (u == NotifBridgeUuids.charOtaData) _otaData = c;
      if (u == NotifBridgeUuids.charFileCtrl) _fileCtrl = c;
      if (u == NotifBridgeUuids.charFileData) _fileData = c;
    }
    // Separate NUS service (VESC-Tool bridge) for the LISP editor — optional.
    _nusRx = null;
    _nusTx = null;
    for (final s in services) {
      if (s.uuid.toString().toLowerCase() != NusUuids.service) continue;
      for (final c in s.characteristics) {
        final u = c.uuid.toString().toLowerCase();
        if (u == NusUuids.rx) _nusRx = c;
        if (u == NusUuids.tx) _nusTx = c;
      }
    }
    await _outbound!.setNotifyValue(true);
    await _outSub?.cancel();
    _outSub = _outbound!.onValueReceived.listen(_onOutbound);
    _setState(BleConnState.connected);
    _startClock();
  }

  /// Push local wall-clock time to the head unit every 15 s (and once
  /// immediately) so its dashboard can show "HH:MM" without an on-device
  /// RTC. No-op when the firmware lacks the time characteristic. The head
  /// unit hides the label on its own if writes stop for 30 s.
  void _startClock() {
    _clockTimer?.cancel();
    if (_time == null) return;
    _pushTime();
    _clockTimer = Timer.periodic(const Duration(seconds: 15), (_) => _pushTime());
  }

  Future<void> _pushTime() async {
    final ch = _time;
    if (ch == null || _state != BleConnState.connected) return;
    final now = DateTime.now();
    try {
      await ch.write([now.hour, now.minute], withoutResponse: true);
    } catch (_) {
      // Transient link hiccup — the next 15 s tick retries; the head unit
      // keeps the last value visible until its 30 s TTL lapses.
    }
  }

  /// Whether the connected head unit exposes the OTA-from-app feature.
  bool get supportsOta => _otaInfo != null;

  /// Read the head unit's SoftAP credentials + OTA endpoint + firmware
  /// version. Returns null if the firmware lacks the characteristic or the
  /// read fails / payload is malformed.
  Future<OtaInfo?> readOtaInfo() async {
    final ch = _otaInfo;
    if (ch == null || _state != BleConnState.connected) return null;
    try {
      final raw = await ch.read();
      return OtaInfo.parse(raw);
    } catch (_) {
      return null;
    }
  }

  /// Whether the connected head unit can be flashed straight over BLE
  /// (firmware exposes the ...0007/...0008 OTA characteristics).
  bool get supportsBleOta => _otaCtrl != null && _otaData != null;

  /// Flash [image] to the head unit over the BLE link (no WiFi/SoftAP hop).
  /// [digest] is the 32-byte SHA-256 of [image]; the head unit verifies it
  /// before committing. [onUpload] reports the upload fraction (0..1) and
  /// [onVerify] fires once the bytes are sent and the device is
  /// writing/verifying. Returns whether the flash succeeded plus, on failure,
  /// an i18n key (see lib/i18n/strings.dart) the UI maps to localized text —
  /// this layer has no BuildContext. Never throws.
  Future<({bool ok, String? errorKey})> bleOta(
    Uint8List image,
    Uint8List digest, {
    void Function(double fraction)? onUpload,
    void Function()? onVerify,
  }) async {
    final ctrl = _otaCtrl;
    final data = _otaData;
    if (ctrl == null || data == null || _state != BleConnState.connected) {
      return (ok: false, errorKey: 'fw.ota.err.noconn');
    }
    if (digest.length != 32) {
      return (ok: false, errorKey: 'fw.ota.err.badsha');
    }

    // Broadcast so we can arm a fresh one-shot listener for each handshake
    // step without re-listening to a single-subscription stream.
    final events = StreamController<({int status, int detail})>.broadcast();
    StreamSubscription<List<int>>? sub;
    try {
      // A 4.4 MB image is thousands of serialized ATT writes, one per
      // connection event: at Android's default ~45 ms interval that is a
      // 10-minute transfer, at high priority (7.5-15 ms) a couple of minutes.
      // The firmware asks for the fast interval from its side too (a
      // peripheral request the phone may ignore); asking here as well makes
      // it stick on Androids that only honour the app's own priority.
      await setLinkSpeed(fast: true);
      await ctrl.setNotifyValue(true);
      sub = ctrl.onValueReceived.listen((raw) {
        if (raw.isEmpty) return;
        final bd = ByteData.sublistView(Uint8List.fromList(raw));
        final status = bd.getUint8(0);
        final detail = raw.length >= 5 ? bd.getUint32(1, Endian.little) : 0;
        events.add((status: status, detail: detail));
      });

      // BEGIN: [op][u32 len LE][32B sha256]. Arm the READY/ERROR future
      // *before* writing so a fast reply can't slip past on the broadcast
      // stream.
      final readyF = events.stream
          .firstWhere((e) =>
              e.status == BleOta.stReady || e.status == BleOta.stError)
          .timeout(const Duration(seconds: 10));
      final begin = Uint8List(1 + 4 + 32);
      begin[0] = BleOta.opBegin;
      ByteData.sublistView(begin, 1, 5).setUint32(0, image.length, Endian.little);
      begin.setRange(5, 37, digest);
      await ctrl.write(begin, withoutResponse: false);

      final ready = await readyF;
      if (ready.status != BleOta.stReady) {
        return (ok: false, errorKey: BleOta.errKey(ready.detail));
      }

      // Stream the image. READY's detail is the largest DATA write the
      // firmware's flatten buffer takes (509 on current firmware; 0 from
      // older builds whose buffer only fits 244) — use the biggest chunk both
      // that and the negotiated MTU allow. Fewer, larger writes matter: every
      // write is a platform round trip plus one ATT packet per connection
      // event, so 509-byte chunks halve the transfer time versus 244.
      final maxChunk = ready.detail > 0 ? ready.detail : 244;
      final mtu = _device?.mtuNow ?? 247;
      final chunkSize = (mtu - 3).clamp(20, maxChunk);
      final total = image.length;
      var off = 0;
      onUpload?.call(0);
      while (off < total) {
        if (_state != BleConnState.connected) {
          return (ok: false, errorKey: 'fw.ota.err.linklost');
        }
        final end = (off + chunkSize) < total ? off + chunkSize : total;
        await _writeOtaData(data, Uint8List.sublistView(image, off, end));
        off = end;
        onUpload?.call(off / total);
      }

      // END: arm the DONE/ERROR future before writing. The device's
      // flash+verify pass takes several seconds.
      onVerify?.call();
      final doneF = events.stream
          .firstWhere((e) =>
              e.status == BleOta.stDone || e.status == BleOta.stError)
          .timeout(const Duration(seconds: 120));
      await ctrl.write(Uint8List.fromList([BleOta.opEnd]), withoutResponse: false);

      final done = await doneF;
      if (done.status == BleOta.stDone) return (ok: true, errorKey: null);
      return (ok: false, errorKey: BleOta.errKey(done.detail));
    } on TimeoutException {
      // Best-effort cancel so a stale staging buffer doesn't linger.
      try {
        await ctrl.write(Uint8List.fromList([BleOta.opAbort]),
            withoutResponse: false);
      } catch (_) {}
      return (ok: false, errorKey: 'fw.ota.err.timeout');
    } catch (_) {
      return (ok: false, errorKey: 'fw.ota.blefail');
    } finally {
      await sub?.cancel();
      await events.close();
      // On success the head unit reboots and the link drops anyway; on
      // failure hand the radio back to normal duty.
      await setLinkSpeed(fast: false);
    }
  }

  /// Write one OTA data chunk. Android reports BUSY (status 201) on a
  /// write-without-response when the controller's TX credits are exhausted;
  /// they come back at the next connection event, so wait roughly one
  /// interval and try again *without* response. Only after a run of misses
  /// fall back to an acknowledged write — that path costs a full ATT round
  /// trip per chunk (one chunk per connection event), and [_send]'s
  /// "fall back after two 15 ms retries" made most of a firmware image go
  /// that slow way whenever the stack was under pressure.
  Future<void> _writeOtaData(BluetoothCharacteristic ch, Uint8List chunk) async {
    const fastAttempts = 6;
    var attempt = 0;
    while (true) {
      try {
        await ch.write(chunk, withoutResponse: attempt < fastAttempts);
        return;
      } on PlatformException catch (e) {
        final msg = (e.message ?? '').toLowerCase();
        final busy = msg.contains('busy') || msg.contains('201');
        if (!busy || attempt >= 12) rethrow;
        attempt++;
        await Future.delayed(Duration(milliseconds: 10 * attempt));
      }
    }
  }

  void _onConnectionStateChanged(BluetoothConnectionState s) {
    if (s == BluetoothConnectionState.connected && _device != null) {
      // The OS-level ACL link just came up (first connect or an autoConnect
      // reconnect). Run the GATT handshake on top of it.
      _handleLinkUp(_device!);
    } else if (s == BluetoothConnectionState.disconnected) {
      _setState(BleConnState.disconnected);
      // autoConnect:true keeps the OS-level reconnect queue active by itself;
      // nothing to retry here. The foreground service stays up regardless (it
      // hosts this whole BLE isolate — see ble_host.dart) so the link can come
      // back, or the user can re-pair, without relaunching the app.
    }
  }

  /// Discover services, request the larger MTU and subscribe to NOTIFY on
  /// top of a freshly-up ACL link. Retries a few times, then — if the
  /// handshake still won't complete — forces a clean disconnect+reconnect.
  ///
  /// This is the fix for "BLE reconnects but no data flows": with
  /// autoConnect:true the stack reconnects the ACL link but never re-emits
  /// `connected`, so a single failed handshake (a transient GATT error, or
  /// a stale service cache after the head unit rebooted) used to leave us
  /// stuck in `disconnected` while the radio link was up — every _send()
  /// then silently no-ops. Now we never give up while a paired device is
  /// reachable; we keep cycling the link until the handshake takes.
  Future<void> _handleLinkUp(BluetoothDevice device) async {
    // Ignore duplicate `connected` events / overlapping attempts.
    if (_handshaking) return;
    _handshaking = true;
    try {
      Object? lastErr;
      for (var attempt = 0; attempt < 3; attempt++) {
        if (_userInitiatedDisconnect) return;
        try {
          await _completeHandshake(device);
          _reconnectAttempt = 0; // success — reset the backoff
          _fg.start();
          _firstHandshake?.complete();
          return;
        } catch (e) {
          lastErr = e;
          await Future.delayed(Duration(milliseconds: 300 * (attempt + 1)));
        }
      }
      // Handshake won't complete on this link.
      _setState(BleConnState.disconnected);
      final fh = _firstHandshake;
      if (fh != null && !fh.isCompleted) {
        // First interactive connect() owns its own 20 s timeout + teardown;
        // surface the error there instead of force-looping underneath it.
        fh.completeError(lastErr ?? StateError('handshake failed'));
        return;
      }
      // Background auto-reconnect: drop the link and reconnect cleanly so
      // Android re-runs service discovery (autoConnect alone won't).
      _scheduleForceReconnect();
    } finally {
      _handshaking = false;
    }
  }

  /// Tear the current link down and re-arm autoConnect after a backoff, so
  /// the next `connected` event runs a fresh GATT discovery. On Android we
  /// also flush the GATT cache first — after the head unit reboots, the
  /// stack can otherwise hand back stale attribute handles and every write
  /// lands nowhere (link up, no data). Capped backoff covers a head unit
  /// that's still booting.
  void _scheduleForceReconnect() {
    if (_userInitiatedDisconnect || _savedRemoteId == null) return;
    _reconnectAttempt++;
    final delayMs = (500 * _reconnectAttempt).clamp(500, 8000);
    _reconnectTimer?.cancel();
    _reconnectTimer = Timer(Duration(milliseconds: delayMs), () async {
      if (_userInitiatedDisconnect || _savedRemoteId == null) return;
      final dev = _device;
      if (dev != null && Platform.isAndroid) {
        try {
          await dev.clearGattCache();
        } catch (_) {}
      }
      try {
        await dev?.disconnect();
      } catch (_) {}
      if (_userInitiatedDisconnect || _savedRemoteId == null) return;
      _attachKnownDevice(_savedRemoteId!);
    });
  }

  /// User-initiated "forget device" — drops the persisted remote id and
  /// tears the link down without re-arming auto-reconnect.
  Future<void> forget() async {
    _userInitiatedDisconnect = true;
    final p = await SharedPreferences.getInstance();
    await p.remove(_prefSavedId);
    _savedRemoteId = null;
    await _teardown();
    // Keep the foreground service alive: it hosts this isolate, and the user
    // may re-pair right away. The notification just flips to "disconnected".
  }

  /// User-initiated "restart BLE" — force a clean reconnect of the saved
  /// device without forgetting it. Unsticks a wedged GATT link (e.g. the app
  /// can't tell which board is connected because a stale service cache handed
  /// back the wrong attributes): drop the link, flush the Android GATT cache
  /// so discovery re-runs, then re-attach. No-op if no device is paired.
  Future<void> restart() async {
    final id = _savedRemoteId;
    if (id == null) return;
    _userInitiatedDisconnect = false;
    _handshaking = false;
    _reconnectTimer?.cancel();
    _reconnectTimer = null;
    _reconnectAttempt = 0;
    _clockTimer?.cancel();
    await _outSub?.cancel();
    _outSub = null;
    await _connSub?.cancel();
    _connSub = null;
    _setState(BleConnState.connecting);
    final dev = _device;
    if (dev != null && Platform.isAndroid) {
      try {
        await dev.clearGattCache();
      } catch (_) {}
    }
    try {
      await dev?.disconnect();
    } catch (_) {}
    // Let the OS tear the ACL link fully down before re-arming autoConnect.
    await Future.delayed(const Duration(milliseconds: 500));
    _attachKnownDevice(id);
  }

  Future<void> _teardown() async {
    _reconnectTimer?.cancel();
    _reconnectTimer = null;
    _reconnectAttempt = 0;
    _handshaking = false;
    _clockTimer?.cancel();
    _clockTimer = null;
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
    _time = null;
    _otaInfo = null;
    _otaCtrl = null;
    _otaData = null;
    _fileCtrl = null;
    _fileData = null;
    _nusRx = null;
    _nusTx = null;
    _setState(BleConnState.idle);
  }

  /// Whether the connected head unit exposes the file manager (...0009/...000a).
  bool get supportsFileManager => _fileCtrl != null && _fileData != null;

  /// Whether the connected head unit exposes the NUS bridge (LISP editor).
  bool get supportsLisp => _nusRx != null && _nusTx != null;

  /// File-manager characteristics + link facts for FileManager. Null/false
  /// when not connected or unsupported.
  BluetoothCharacteristic? get fileCtrlChar => _fileCtrl;
  BluetoothCharacteristic? get fileDataChar => _fileData;

  /// NUS bridge characteristics for the LISP editor (see lib/ble/vesc/).
  BluetoothCharacteristic? get nusRxChar => _nusRx;
  BluetoothCharacteristic? get nusTxChar => _nusTx;
  bool get isConnected => _state == BleConnState.connected;
  int get negotiatedMtu => _device?.mtuNow ?? 247;

  /// Ask Android for a faster (high-priority, 7.5-15 ms interval) or normal
  /// (balanced) connection. Long LISP code transfers are dozens of serialized
  /// round-trips, and the Android default ~45 ms interval dominates their
  /// wall-clock time — high priority speeds them up several times over.
  /// Best-effort: silently a no-op off-Android or when disconnected.
  Future<void> setLinkSpeed({required bool fast}) async {
    final dev = _device;
    if (dev == null || !isConnected) return;
    try {
      await dev.requestConnectionPriority(
          connectionPriorityRequest:
              fast ? ConnectionPriority.high : ConnectionPriority.balanced);
    } catch (_) {
      // android-only API / device just disconnected — speed is a hint anyway
    }
  }

  Future<void> sendNotification(NotificationMsg n) =>
      _send(PduType.notification, n.encode());

  Future<void> sendMedia(MediaMsg m) => _send(PduType.media, m.encode());

  Future<void> sendIcon(IconMsg i) => _send(PduType.icon, i.encode());

  Future<void> sendAlbumArt(IconMsg i) => _send(PduType.albumArt, i.encode());

  /// Serializes outbound chunks. Drops the whole PDU if the link goes
  /// away mid-transfer; never throws — exceptions bubble up as unhandled
  /// otherwise (Coordinator fires this from a stream listener with no
  /// outer try/catch).
  ///
  /// flutter_blue_plus's `withoutResponse:true` path doesn't internally
  /// queue writes against the GATT stack — back-to-back chunks of a
  /// 50-90 KB album-art PNG slam Android's ATT layer faster than it can
  /// drain and we get `ERROR_GATT_WRITE_REQUEST_BUSY` (status 201).
  /// Retry with a short backoff handles the transient pressure; falling
  /// back to `withoutResponse:false` after a couple of fails forces the
  /// stack to wait for ACKs, slower but unbreakable.
  Future<void> _send(PduType type, Uint8List body) async {
    if (_state != BleConnState.connected) return;
    final ch = _inbound;
    if (ch == null) return;
    final chunks = _encoder.encode(type, body);
    for (final c in chunks) {
      var attempt = 0;
      while (true) {
        try {
          // First two tries: fast write-without-response. After that
          // switch to acknowledged writes which the stack can pace.
          final wor = attempt < 2;
          await ch.write(c, withoutResponse: wor);
          break;
        } on PlatformException catch (e) {
          final msg = (e.message ?? '').toLowerCase();
          final busy = msg.contains('busy') || msg.contains('201');
          if (!busy || attempt >= 6) {
            // Permanent error or out of retries — drop the rest of the
            // PDU; the head unit will time it out / reasm-drop the
            // partial.
            return;
          }
          attempt++;
          await Future.delayed(Duration(milliseconds: 20 * attempt));
        } catch (_) {
          // Anything else (link gone, channel missing) — bail silently.
          return;
        }
      }
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
    // Mirror the state into the system shade pill so the user sees
    // "Connected / Disconnected" without opening the app.
    final connected = s == BleConnState.connected;
    final subtitle = switch (s) {
      BleConnState.connected =>
        _device?.platformName.isNotEmpty == true
            ? 'Linked to ${_device!.platformName}'
            : 'Linked to head unit',
      BleConnState.connecting => 'Connecting…',
      BleConnState.scanning => 'Scanning…',
      BleConnState.disconnected => '',
      BleConnState.idle => '',
    };
    _fg.setStatus(connected: connected, subtitle: subtitle).catchError((_) {});
  }
}
