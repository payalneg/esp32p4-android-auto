/// GATT protocol of the VESC BLE Helper (XIAO ESP32-C3) — the little board
/// that bridges sleepy BLE gadgets (remote buttons, a cadence sensor) to the
/// VESC over CAN.
///
/// Ported from the helper's own configurator app; the layouts mirror the
/// firmware's `settings.h` and `tools/config_gui.py` (little-endian, packed).
/// Versioned blobs are rejected rather than mis-parsed when the firmware moves
/// on — [PasParams.decode] returns null on an unknown version.
///
/// Note what is NOT here: the helper's NUS bridge to the VESC. It advertises
/// the standard Nordic UART service, so the existing [DirectNusTransport] in
/// the LISP editor already talks to a VESC through it — no second
/// implementation needed.
library;

import 'dart:convert';
import 'dart:typed_data';

const kHelperDeviceName = 'VESC-BLE-Helper';

/// The helper advertises the NUS service UUID; its name only appears in the
/// scan response, so a phone-side scan filters on this.
const kHelperNusServiceUuid = '6e400001-b5a3-f393-e0a9-e50e24dcca9e';

const kCfgCtrlUuid = 'ab1e0002-b1e5-4e15-8ac3-5e00c0de15b7';
const kCfgStatusUuid = 'ab1e0003-b1e5-4e15-8ac3-5e00c0de15b7';
const kCfgScanUuid = 'ab1e0004-b1e5-4e15-8ac3-5e00c0de15b7';
const kHelperOtaCtrlUuid = 'ab1e0005-b1e5-4e15-8ac3-5e00c0de15b7';
const kHelperOtaDataUuid = 'ab1e0006-b1e5-4e15-8ac3-5e00c0de15b7';

/// Device Information Service: Firmware Revision String. Read on connect and
/// compared with the bundled image; absent on firmware < 1.0.1.
const kFwRevisionUuid = '2a26';

/// Config-service CTRL commands.
abstract final class HelperCmd {
  static const scan = 1;
  static const bindButton = 2;
  static const bindCadence = 3;
  static const unbind = 4;
  static const getParams = 5;
  static const setParams = 6;
  static const setThrottle = 7;
  static const setBinding = 8;
  static const getBinding = 9;
}

const kWhatButton = 1;
const kWhatCadence = 2;

/// OTA ops and notification statuses.
abstract final class HelperOta {
  static const opBegin = 1;
  static const opEnd = 2;
  static const opAbort = 3;
  static const stReady = 0x10;
  static const stProgress = 0x11;
  static const stDone = 0x12;
  static const stError = 0x1F;
}

/// `btn_action_t` (settings.h): every button fires a configurable raw CAN
/// frame; what it MEANS lives in the LISP script on the VESC.
const kBtnActCustomCan = 4;
const kBtnUiSlots = 8;

const kCanSpeeds = [125, 250, 500, 1000];

/// PAS/CAN parameter blob, version 3 (35 bytes).
class PasParams {
  static const version = 3;
  static const length = 35;

  int enabled = 1;
  int reverse = 0;
  int level = 1;
  int levelCount = 3;
  int mode = 0;
  int startCurrentPct = 0;
  int startDelayMs = 0;
  int stopDelayMs = 0;
  int minCadenceRpm = 0;
  int fullCadenceRpm = 0;
  int maxCurrentMa = 0;
  int rampUpMaps = 0; // mA per second
  int controllerId = 0;
  int targetVescId = 0;
  List<int> btnActions = List.filled(kBtnUiSlots, kBtnActCustomCan);
  int canKbps = 500;

  Uint8List encode() {
    final b = ByteData(length);
    b.setUint8(0, version);
    b.setUint8(1, enabled);
    b.setUint8(2, reverse);
    b.setUint8(3, level);
    b.setUint8(4, levelCount);
    b.setUint8(5, mode);
    b.setUint8(6, startCurrentPct);
    b.setUint16(7, startDelayMs, Endian.little);
    b.setUint16(9, stopDelayMs, Endian.little);
    b.setUint16(11, minCadenceRpm, Endian.little);
    b.setUint16(13, fullCadenceRpm, Endian.little);
    b.setUint32(15, maxCurrentMa, Endian.little);
    b.setUint32(19, rampUpMaps, Endian.little);
    b.setUint8(23, controllerId);
    b.setUint8(24, targetVescId);
    for (var i = 0; i < kBtnUiSlots; i++) {
      b.setUint8(25 + i, btnActions[i]);
    }
    b.setUint16(33, canKbps, Endian.little);
    return b.buffer.asUint8List();
  }

  /// Null when the blob is short or carries a version this build can't read —
  /// better to say "unsupported" than to write back a mis-parsed struct.
  static PasParams? decode(Uint8List blob) {
    if (blob.length < length) return null;
    final b = ByteData.sublistView(blob);
    if (b.getUint8(0) != version) return null;
    return PasParams()
      ..enabled = b.getUint8(1)
      ..reverse = b.getUint8(2)
      ..level = b.getUint8(3)
      ..levelCount = b.getUint8(4)
      ..mode = b.getUint8(5)
      ..startCurrentPct = b.getUint8(6)
      ..startDelayMs = b.getUint16(7, Endian.little)
      ..stopDelayMs = b.getUint16(9, Endian.little)
      ..minCadenceRpm = b.getUint16(11, Endian.little)
      ..fullCadenceRpm = b.getUint16(13, Endian.little)
      ..maxCurrentMa = b.getUint32(15, Endian.little)
      ..rampUpMaps = b.getUint32(19, Endian.little)
      ..controllerId = b.getUint8(23)
      ..targetVescId = b.getUint8(24)
      ..btnActions = [for (var i = 0; i < kBtnUiSlots; i++) b.getUint8(25 + i)]
      ..canKbps = b.getUint16(33, Endian.little);
  }

  /// The struct crosses the isolate port as base64 rather than as a map: it
  /// is a firmware-defined blob, and re-describing it in JSON would be a
  /// second layout to keep in sync.
  String toB64() => base64Encode(encode());
  static PasParams? fromB64(String s) => decode(base64Decode(s));

  PasParams copy() => PasParams.decode(encode())!;
}

/// Live status notification, version 2 (12 bytes).
class HelperStatus {
  static const version = 2;
  static const length = 12;

  final int flags;
  final double rpm;
  final int batt;
  final int level;
  final double assistA;
  final int btnMask;
  final int btnCount;

  const HelperStatus({
    required this.flags,
    required this.rpm,
    required this.batt,
    required this.level,
    required this.assistA,
    required this.btnMask,
    required this.btnCount,
  });

  bool get cadenceBound => flags & (1 << 0) != 0;
  bool get cadenceConnected => flags & (1 << 1) != 0;
  bool get remoteBound => flags & (1 << 2) != 0;
  bool get remoteConnected => flags & (1 << 3) != 0;
  bool get throttleOn => flags & (1 << 5) != 0;
  bool get vescLink => flags & (1 << 6) != 0;
  bool get pasEnabled => flags & (1 << 7) != 0;

  /// Battery reports 0xFF when the sensor hasn't said.
  bool get battKnown => batt != 0xFF;

  static HelperStatus? decode(Uint8List data) {
    if (data.length < length) return null;
    final b = ByteData.sublistView(data);
    if (b.getUint8(0) != version) return null;
    return HelperStatus(
      flags: b.getUint8(1),
      rpm: b.getInt16(2, Endian.little) / 100.0,
      batt: b.getUint8(4),
      level: b.getUint8(5),
      assistA: b.getInt32(6, Endian.little) / 1000.0,
      btnMask: b.getUint8(10),
      btnCount: b.getUint8(11),
    );
  }

  Map<String, dynamic> toMap() => {
        'flags': flags,
        'rpm': rpm,
        'batt': batt,
        'level': level,
        'assistA': assistA,
        'btnMask': btnMask,
        'btnCount': btnCount,
      };

  factory HelperStatus.fromMap(Map<String, dynamic> m) => HelperStatus(
        flags: (m['flags'] as num?)?.toInt() ?? 0,
        rpm: (m['rpm'] as num?)?.toDouble() ?? 0,
        batt: (m['batt'] as num?)?.toInt() ?? 0xFF,
        level: (m['level'] as num?)?.toInt() ?? 0,
        assistA: (m['assistA'] as num?)?.toDouble() ?? 0,
        btnMask: (m['btnMask'] as num?)?.toInt() ?? 0,
        btnCount: (m['btnCount'] as num?)?.toInt() ?? 0,
      );
}

/// One hit from the scan the HELPER runs (not the phone): it hunts for
/// sleepy gadgets that the phone may never see.
class ScanHit {
  final int what; // kWhatButton / kWhatCadence
  final int addrType;

  /// Raw NimBLE byte order — passed back verbatim on bind, never reordered.
  final Uint8List addr;
  final int rssi;
  final String name;

  const ScanHit(this.what, this.addrType, this.addr, this.rssi, this.name);

  /// NimBLE stores the address least-significant byte first.
  String get mac => addr.reversed
      .map((b) => b.toRadixString(16).padLeft(2, '0').toUpperCase())
      .join(':');

  static ScanHit? decode(Uint8List data) {
    if (data.length < 10) return null;
    final nameLen = data[9];
    final name = utf8.decode(
        data.sublist(10, (10 + nameLen).clamp(0, data.length)),
        allowMalformed: true);
    return ScanHit(data[0], data[1], Uint8List.fromList(data.sublist(2, 8)),
        ByteData.sublistView(data).getInt8(8), name);
  }

  Map<String, dynamic> toMap() => {
        'what': what,
        'addrType': addrType,
        'addr': base64Encode(addr),
        'rssi': rssi,
        'name': name,
      };

  factory ScanHit.fromMap(Map<String, dynamic> m) => ScanHit(
        (m['what'] as num?)?.toInt() ?? 0,
        (m['addrType'] as num?)?.toInt() ?? 0,
        base64Decode(m['addr'] as String? ?? ''),
        (m['rssi'] as num?)?.toInt() ?? 0,
        m['name'] as String? ?? '',
      );
}

/// Per-button custom CAN frame. The helper only ships the frame; the meaning
/// is whatever the LISP script on the VESC makes of it.
class ButtonBinding {
  final int idx;
  final int ext;
  final int canId;
  final Uint8List data;

  const ButtonBinding(this.idx, this.ext, this.canId, this.data);

  static ButtonBinding? decode(Uint8List d) {
    if (d.length < 16) return null;
    final len = d[3] > 8 ? 8 : d[3];
    final canId = ByteData.sublistView(d).getUint32(4, Endian.little);
    return ButtonBinding(
        d[1], d[2], canId, Uint8List.fromList(d.sublist(8, 8 + len)));
  }

  /// CMD_SET_BINDING payload: [cmd, idx, ext, len] + u32le id + data padded
  /// to 8 bytes.
  Uint8List encodeSet() {
    final out = Uint8List(4 + 4 + 8);
    out[0] = HelperCmd.setBinding;
    out[1] = idx;
    out[2] = ext;
    out[3] = data.length > 8 ? 8 : data.length;
    ByteData.sublistView(out).setUint32(4, canId, Endian.little);
    out.setRange(8, 8 + out[3], data);
    return out;
  }

  Map<String, dynamic> toMap() => {
        'idx': idx,
        'ext': ext,
        'canId': canId,
        'data': base64Encode(data),
      };

  factory ButtonBinding.fromMap(Map<String, dynamic> m) => ButtonBinding(
        (m['idx'] as num?)?.toInt() ?? 0,
        (m['ext'] as num?)?.toInt() ?? 0,
        (m['canId'] as num?)?.toInt() ?? 0,
        base64Decode(m['data'] as String? ?? ''),
      );
}

String hexBytes(List<int> data) => data
    .map((b) => b.toRadixString(16).padLeft(2, '0').toUpperCase())
    .join();

/// Strict hex → bytes: spaces allowed, throws on odd length or bad digits.
Uint8List bytesFromHex(String hex) {
  final clean = hex.replaceAll(' ', '');
  if (clean.length.isOdd) {
    throw const FormatException('odd-length hex string');
  }
  return Uint8List.fromList([
    for (var i = 0; i < clean.length; i += 2)
      int.parse(clean.substring(i, i + 2), radix: 16),
  ]);
}
