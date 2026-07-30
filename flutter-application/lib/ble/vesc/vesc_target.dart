/// Which NUS link the LISP editor is pointed at, and its lifecycle.
///
/// Two possible targets (see [NusTransport]): the head unit's bridge, or a
/// stand-alone VESC BLE adapter connected by the app itself. The choice is
/// persisted, but a direct adapter link is only *held* while the editor is open
/// — [resume] brings it up when the screen opens, [release] drops it on close,
/// so the radio isn't tied up the rest of the time.
///
/// Background BLE isolate only; the UI sees a [VescTargetInfo] over the port.
library;

import 'dart:async';

import 'package:shared_preferences/shared_preferences.dart';

import '../ble_service.dart';
import '../lisp_models.dart';
import 'nus_transport.dart';
import 'vesc_errors.dart';

class VescTarget {
  VescTarget._();
  static final VescTarget instance = VescTarget._();

  static const _prefKind = 'vesc_nus_kind_v1';
  static const _prefId = 'vesc_nus_remote_id_v1';
  static const _prefName = 'vesc_nus_name_v1';

  VescTargetKind _kind = VescTargetKind.headUnit;
  String? _savedId;
  String? _savedName;
  DirectNusTransport? _direct;
  StreamSubscription<VescLinkState>? _directSub;

  final _changes = StreamController<VescTargetInfo>.broadcast();

  /// Emits whenever the target or its link state changes.
  Stream<VescTargetInfo> get changes => _changes.stream;

  VescTargetKind get kind => _kind;

  /// Head unit reachable *and* exposing the NUS bridge.
  bool get headUnitAvailable => HeadUnitNusTransport.instance.available;

  /// Whether a LISP operation could run right now.
  bool get available => info.connected;

  VescTargetInfo get info {
    if (_kind == VescTargetKind.direct) {
      final d = _direct;
      return VescTargetInfo(
        kind: VescTargetKind.direct,
        remoteId: d?.remoteId ?? _savedId,
        name: d?.name ?? _savedName,
        state: d?.state ?? VescLinkState.idle,
        mtu: d?.mtu ?? 0,
        headUnitAvailable: headUnitAvailable,
      );
    }
    return VescTargetInfo(
      kind: VescTargetKind.headUnit,
      state: headUnitAvailable ? VescLinkState.connected : VescLinkState.idle,
      mtu: headUnitAvailable ? BleService.instance.negotiatedMtu : 0,
      headUnitAvailable: headUnitAvailable,
    );
  }

  void _notify() {
    if (!_changes.isClosed) _changes.add(info);
  }

  /// Load the persisted choice. Does NOT connect — [resume] does that when the
  /// editor opens. Call once from the task handler's `onStart`.
  Future<void> restore() async {
    final p = await SharedPreferences.getInstance();
    _kind = p.getString(_prefKind) == VescTargetKind.direct.name
        ? VescTargetKind.direct
        : VescTargetKind.headUnit;
    _savedId = p.getString(_prefId);
    _savedName = p.getString(_prefName);
    if (_kind == VescTargetKind.direct && _savedId == null) {
      _kind = VescTargetKind.headUnit;
    }
    _notify();
  }

  Future<void> _persist() async {
    final p = await SharedPreferences.getInstance();
    await p.setString(_prefKind, _kind.name);
    if (_savedId != null) {
      await p.setString(_prefId, _savedId!);
      await p.setString(_prefName, _savedName ?? '');
    }
  }

  /// Point at the head unit's bridge (drops any direct link).
  Future<void> selectHeadUnit() async {
    _kind = VescTargetKind.headUnit;
    await _dropDirect();
    await _persist();
    _notify();
    if (!headUnitAvailable) {
      throw const VescLispException('lisp.err.noconn');
    }
  }

  /// Point at (and connect to) a stand-alone NUS adapter.
  Future<void> selectDirect(String remoteId, String name) async {
    if (_direct != null && _direct!.remoteId != remoteId) {
      await _dropDirect();
    }
    _kind = VescTargetKind.direct;
    _savedId = remoteId;
    _savedName = name;
    await _persist();
    _notify();
    await _connectDirect(remoteId, name);
  }

  /// Re-apply the persisted choice — called when the editor screen opens.
  /// Reconnects the saved adapter; a head-unit target needs nothing.
  Future<void> resume() async {
    if (_kind != VescTargetKind.direct) {
      _notify();
      return;
    }
    final id = _savedId;
    if (id == null) {
      _kind = VescTargetKind.headUnit;
      _notify();
      return;
    }
    await _connectDirect(id, _savedName ?? '');
  }

  Future<void> _connectDirect(String remoteId, String name) async {
    var d = _direct;
    if (d == null) {
      d = DirectNusTransport(remoteId: remoteId, name: name);
      _direct = d;
      _directSub = d.states.listen((_) => _notify());
    }
    _notify();
    try {
      await d.connect();
    } finally {
      _notify();
    }
  }

  /// Let go of the direct adapter (editor closed). Keeps the saved choice so
  /// the next [resume] reconnects the same adapter.
  Future<void> release() async {
    await _dropDirect();
    _notify();
  }

  Future<void> _dropDirect() async {
    final d = _direct;
    _direct = null;
    await _directSub?.cancel();
    _directSub = null;
    if (d != null) await d.dispose();
  }

  /// The active transport, ready to use. Throws [VescLispException] otherwise.
  Future<NusTransport> require() async {
    final NusTransport t;
    if (_kind == VescTargetKind.direct) {
      final d = _direct;
      if (d == null) {
        // Editor open but the link was never brought up (e.g. after a service
        // restart) — bring it up now rather than failing.
        final id = _savedId;
        if (id == null) throw const VescLispException('lisp.err.notarget');
        await _connectDirect(id, _savedName ?? '');
        t = _direct ?? (throw const VescLispException('lisp.err.notarget'));
      } else {
        t = d;
      }
    } else {
      t = HeadUnitNusTransport.instance;
    }
    await t.ensureReady();
    return t;
  }
}
