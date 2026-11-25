/// The bike's pack charge as the head unit reports it over the NotifBridge
/// battery characteristic (`...000d`).
///
/// The head unit is the only thing that talks to the VESC/BMS, so this is the
/// same percentage its dashboard shows. A reading is cached in
/// SharedPreferences: the bike is usually switched off by the time its rider
/// wants to look at the charge, and "last read N hours ago" is still the answer
/// to "how much is left?".
library;

import 'dart:typed_data';

import 'package:shared_preferences/shared_preferences.dart';

class BikeBattery {
  /// Wire format, version 1 (see main/notif_bridge.c):
  ///   u8  version
  ///   u8  flags (bit0 SOC, bit1 voltage, bit2 remaining Ah)
  ///   u8  SOC %
  ///   u8  reserved
  ///   u16 voltage, 0.01 V
  ///   u16 remaining capacity, 0.01 Ah
  static const wireVersion = 1;
  static const wireLength = 8;

  static const prefPercent = 'bike_batt_pct_v1';
  static const prefVoltage = 'bike_batt_voltage_v1';
  static const prefRemainingAh = 'bike_batt_ah_v1';
  static const prefAt = 'bike_batt_at_v1';

  final int? percent;
  final double? voltageV;
  final double? remainingAh;
  final DateTime at;

  /// True when this came from a read on the current link; false when it was
  /// restored from the cache because the head unit is away.
  final bool live;

  const BikeBattery({
    required this.percent,
    required this.voltageV,
    required this.remainingAh,
    required this.at,
    required this.live,
  });

  BikeBattery copyWith({bool? live}) => BikeBattery(
    percent: percent,
    voltageV: voltageV,
    remainingAh: remainingAh,
    at: at,
    live: live ?? this.live,
  );

  /// Null when the payload is not ours or carries no usable field at all.
  static BikeBattery? parse(List<int> raw, {DateTime? at}) {
    if (raw.length < wireLength) return null;
    final data = Uint8List.fromList(raw);
    if (data[0] != wireVersion) return null;
    final b = ByteData.sublistView(data);
    final flags = b.getUint8(1);

    int? percent;
    if (flags & 0x01 != 0) {
      final p = b.getUint8(2);
      if (p <= 100) percent = p;
    }
    double? voltage;
    if (flags & 0x02 != 0) {
      final centiVolts = b.getUint16(4, Endian.little);
      if (centiVolts > 0) voltage = centiVolts / 100.0;
    }
    double? remainingAh;
    if (flags & 0x04 != 0) {
      remainingAh = b.getUint16(6, Endian.little) / 100.0;
    }
    if (percent == null && voltage == null && remainingAh == null) return null;
    final when = at ?? DateTime.now();
    return BikeBattery(
      percent: percent,
      voltageV: voltage,
      remainingAh: remainingAh,
      // The wire/IPC/prefs path is milliseconds; keep one precision so a
      // round-trip compares equal.
      at: DateTime.fromMillisecondsSinceEpoch(when.millisecondsSinceEpoch),
      live: true,
    );
  }

  Map<String, dynamic> toMap() => {
    'pct': percent,
    'v': voltageV,
    'ah': remainingAh,
    'at': at.millisecondsSinceEpoch,
    'live': live,
  };

  factory BikeBattery.fromMap(Map<String, dynamic> m) => BikeBattery(
    percent: (m['pct'] as num?)?.toInt(),
    voltageV: (m['v'] as num?)?.toDouble(),
    remainingAh: (m['ah'] as num?)?.toDouble(),
    at: DateTime.fromMillisecondsSinceEpoch((m['at'] as num?)?.toInt() ?? 0),
    live: m['live'] as bool? ?? false,
  );

  /// Restore the last reading written by either isolate. Null when nothing was
  /// ever read.
  static BikeBattery? loadFrom(SharedPreferences p) {
    final at = p.getInt(prefAt);
    if (at == null || at <= 0) return null;
    var percent = p.getInt(prefPercent);
    if (percent != null && (percent < 0 || percent > 100)) percent = null;
    final voltageRaw = p.get(prefVoltage);
    var voltage = voltageRaw is num ? voltageRaw.toDouble() : null;
    if (voltage != null && voltage <= 0) voltage = null;
    final ahRaw = p.get(prefRemainingAh);
    var remainingAh = ahRaw is num ? ahRaw.toDouble() : null;
    if (remainingAh != null && remainingAh < 0) remainingAh = null;
    if (percent == null && voltage == null && remainingAh == null) return null;
    return BikeBattery(
      percent: percent,
      voltageV: voltage,
      remainingAh: remainingAh,
      at: DateTime.fromMillisecondsSinceEpoch(at),
      live: false,
    );
  }

  Future<void> saveTo(SharedPreferences p) async {
    await p.setInt(prefPercent, percent ?? -1);
    await p.setDouble(prefVoltage, voltageV ?? -1);
    await p.setDouble(prefRemainingAh, remainingAh ?? -1);
    await p.setInt(prefAt, at.millisecondsSinceEpoch);
  }
}
