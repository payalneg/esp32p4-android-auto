/// Codec + cache tests for the bike battery reading.
library;

import 'dart:typed_data';

import 'package:aa_bridge/ble/bike_battery.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:shared_preferences/shared_preferences.dart';

void main() {
  Uint8List frame({
    int version = BikeBattery.wireVersion,
    int flags = 0x01 | 0x02 | 0x04,
    int percent = 73,
    int centiVolts = 5400,
    int centiAh = 1200,
  }) {
    final b = ByteData(BikeBattery.wireLength);
    b.setUint8(0, version);
    b.setUint8(1, flags);
    b.setUint8(2, percent);
    b.setUint16(4, centiVolts, Endian.little);
    b.setUint16(6, centiAh, Endian.little);
    return b.buffer.asUint8List();
  }

  group('BikeBattery.parse', () {
    test('decodes percent, voltage and remaining capacity', () {
      final b = BikeBattery.parse(frame())!;
      expect(b.percent, 73);
      expect(b.voltageV, closeTo(54.0, 0.001));
      expect(b.remainingAh, closeTo(12.0, 0.001));
      expect(b.live, isTrue);
    });

    test('accepts a partial frame and rejects one with no fields', () {
      final v = BikeBattery.parse(frame(flags: 0x02, centiVolts: 4823))!;
      expect(v.percent, isNull);
      expect(v.voltageV, closeTo(48.23, 0.001));
      expect(BikeBattery.parse(frame(flags: 0)), isNull);
    });

    test('rejects another wire version and a runt', () {
      expect(BikeBattery.parse(frame(version: 2)), isNull);
      expect(BikeBattery.parse(Uint8List(4)), isNull);
    });

    test('75.5 V does not overflow the centivolt field', () {
      final b = BikeBattery.parse(frame(centiVolts: 7550))!;
      expect(b.voltageV, closeTo(75.5, 0.001));
    });

    test('crosses the isolate port as a map', () {
      final b = BikeBattery.parse(frame())!;
      final back = BikeBattery.fromMap(b.toMap());
      expect(back.percent, 73);
      expect(back.voltageV, closeTo(54.0, 0.001));
      expect(back.remainingAh, closeTo(12.0, 0.001));
      expect(back.at, b.at);
      expect(back.live, isTrue);
    });
  });

  group('cache', () {
    setUp(() => SharedPreferences.setMockInitialValues({}));

    test('round-trips through preferences as a last-known reading', () async {
      final p = await SharedPreferences.getInstance();
      await BikeBattery.parse(frame())!.saveTo(p);

      final back = BikeBattery.loadFrom(p)!;
      expect(back.percent, 73);
      expect(back.voltageV, closeTo(54.0, 0.001));
      expect(back.remainingAh, closeTo(12.0, 0.001));
      expect(back.live, isFalse);
    });

    test('an empty cache is null, not a zero-percent bike', () async {
      final p = await SharedPreferences.getInstance();
      expect(BikeBattery.loadFrom(p), isNull);
    });
  });
}
