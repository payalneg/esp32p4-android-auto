/// UI-side handle for the VESC BLE Helper.
///
/// Mirrors [LispProxy]: the real BLE work happens in the background isolate
/// ([HelperService]); this re-types the JSON that comes back over the port and
/// keeps the last known state so a freshly-built screen paints immediately.
///
/// A [ChangeNotifier] because the helper screen is a handful of tabs all
/// looking at the same live state.
library;

import 'dart:async';
import 'dart:convert';

import 'package:flutter/foundation.dart';

import '../ble/ble_proxy.dart';
import '../ble/ipc.dart';
import 'helper_protocol.dart';
import 'helper_service.dart' show HelperConnState;

class HelperProxy extends ChangeNotifier {
  HelperProxy._() {
    _sub = BleProxy.instance.helperEvents.listen(_onEvent);
  }
  static final HelperProxy instance = HelperProxy._();

  final _ble = BleProxy.instance;
  late final StreamSubscription<Map<String, dynamic>> _sub;

  HelperConnState state = HelperConnState.idle;
  String? fwVersion;
  bool busy = false;

  HelperStatus? status;
  PasParams? params;

  /// idx → binding, filled in as the helper answers GET_BINDING.
  final bindings = <int, ButtonBinding>{};

  /// Hits from the helper-side scan, newest first, deduplicated by address.
  final scanHits = <ScanHit>[];
  bool scanning = false;

  final log = <String>[];
  static const _logLimit = 200;

  double? otaProgress;

  bool get connected => state == HelperConnState.connected;

  final _scanHitCtrl = StreamController<ScanHit>.broadcast();
  Stream<ScanHit> get onScanHit => _scanHitCtrl.stream;

  void _onEvent(Map<String, dynamic> m) {
    switch (m['t']) {
      case IpcEvt.helperState:
        state = HelperConnState.values.firstWhere(
            (e) => e.name == m['state'],
            orElse: () => HelperConnState.idle);
        fwVersion = m['fw'] as String?;
        busy = m['busy'] as bool? ?? false;
        if (!connected) {
          // Stale state after a drop is worse than none: the screen should
          // say "not connected", not show yesterday's cadence.
          status = null;
          params = null;
          bindings.clear();
          otaProgress = null;
        }
      case IpcEvt.helperStatusFrame:
        final s = m['status'];
        if (s is Map) {
          status = HelperStatus.fromMap(Map<String, dynamic>.from(s));
        }
      case IpcEvt.helperParams:
        params = PasParams.fromB64(m['b64'] as String? ?? '');
        if (params == null) _push('unsupported parameter version');
      case IpcEvt.helperBinding:
        final b = m['binding'];
        if (b is Map) {
          final binding =
              ButtonBinding.fromMap(Map<String, dynamic>.from(b));
          bindings[binding.idx] = binding;
        }
      case IpcEvt.helperScanHit:
        final h = m['hit'];
        if (h is Map) {
          final hit = ScanHit.fromMap(Map<String, dynamic>.from(h));
          scanHits.removeWhere((e) => e.mac == hit.mac && e.what == hit.what);
          scanHits.insert(0, hit);
          _scanHitCtrl.add(hit);
        }
      case IpcEvt.helperLog:
        _push(m['line'] as String? ?? '');
      case IpcEvt.helperOtaProgress:
        otaProgress = (m['frac'] as num?)?.toDouble();
    }
    notifyListeners();
  }

  void _push(String line) {
    if (line.isEmpty) return;
    log.add(line);
    if (log.length > _logLimit) log.removeRange(0, log.length - _logLimit);
  }

  // ---- commands ----

  Future<void> connect({String? remoteId}) async {
    scanning = true;
    notifyListeners();
    try {
      final r = await _ble.fileRequest(
          IpcCmd.helperConnect, {if (remoteId != null) 'remoteId': remoteId});
      _applyStatus(r);
    } finally {
      scanning = false;
      notifyListeners();
    }
  }

  Future<void> disconnect() async {
    _applyStatus(await _ble.fileRequest(IpcCmd.helperDisconnect, {}));
  }

  /// Pull the live state — the background isolate only pushes on change, so a
  /// screen opened after the fact would otherwise show "not connected".
  Future<void> refresh() async {
    _applyStatus(await _ble.fileRequest(IpcCmd.helperStatus, {}));
    if (connected) await getParams();
  }

  void _applyStatus(Map<String, dynamic> r) {
    state = HelperConnState.values
        .firstWhere((e) => e.name == r['state'], orElse: () => state);
    fwVersion = r['fw'] as String? ?? fwVersion;
    busy = r['busy'] as bool? ?? busy;
    notifyListeners();
  }

  /// Ask the HELPER to scan (it takes ~6 s and finds sleepy gadgets the phone
  /// never sees).
  Future<void> scanRemotes(int what) {
    scanHits.removeWhere((h) => h.what == what);
    notifyListeners();
    return _ble.fileRequest(IpcCmd.helperScan, {'what': what});
  }

  Future<void> bind(ScanHit hit) =>
      _ble.fileRequest(IpcCmd.helperBind, {'hit': hit.toMap()});

  Future<void> unbind(int what) =>
      _ble.fileRequest(IpcCmd.helperUnbind, {'what': what});

  Future<void> getParams() => _ble.fileRequest(IpcCmd.helperGetParams, {});

  Future<void> setParams(PasParams p) =>
      _ble.fileRequest(IpcCmd.helperSetParams, {'b64': p.toB64()});

  /// 0 off, 1 on, 0xFF toggle.
  Future<void> throttle(int value) =>
      _ble.fileRequest(IpcCmd.helperThrottle, {'value': value});

  Future<void> setBinding(ButtonBinding b) =>
      _ble.fileRequest(IpcCmd.helperSetBinding, {'binding': b.toMap()});

  Future<void> flashFirmware(Uint8List image) async {
    otaProgress = 0;
    notifyListeners();
    try {
      await _ble.fileRequest(IpcCmd.helperOta, {'b64': base64Encode(image)});
    } finally {
      otaProgress = null;
      notifyListeners();
    }
  }

  @override
  void dispose() {
    _sub.cancel();
    _scanHitCtrl.close();
    super.dispose();
  }
}
