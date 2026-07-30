/// Codec tests for the VESC BLE Helper protocol.
///
/// These blobs are firmware-defined structs: a silent layout drift writes
/// nonsense into a device that then drives a motor. The round-trips below are
/// what catches that, and the version guards are what make a mismatch loud.
library;

import 'dart:typed_data';

import 'package:aa_bridge/helper/helper_protocol.dart';
import 'package:flutter_test/flutter_test.dart';

void main() {
  group('PasParams', () {
    test('round-trips every field', () {
      final p = PasParams()
        ..enabled = 1
        ..reverse = 1
        ..level = 2
        ..levelCount = 5
        ..mode = 1
        ..startCurrentPct = 42
        ..startDelayMs = 300
        ..stopDelayMs = 700
        ..minCadenceRpm = 15
        ..fullCadenceRpm = 90
        ..maxCurrentMa = 27500
        ..rampUpMaps = 12000
        ..controllerId = 7
        ..targetVescId = 21
        ..btnActions = List.generate(kBtnUiSlots, (i) => i)
        ..canKbps = 250;

      final back = PasParams.decode(p.encode())!;
      expect(back.enabled, 1);
      expect(back.reverse, 1);
      expect(back.level, 2);
      expect(back.levelCount, 5);
      expect(back.mode, 1);
      expect(back.startCurrentPct, 42);
      expect(back.startDelayMs, 300);
      expect(back.stopDelayMs, 700);
      expect(back.minCadenceRpm, 15);
      expect(back.fullCadenceRpm, 90);
      expect(back.maxCurrentMa, 27500);
      expect(back.rampUpMaps, 12000);
      expect(back.controllerId, 7);
      expect(back.targetVescId, 21);
      expect(back.btnActions, List.generate(kBtnUiSlots, (i) => i));
      expect(back.canKbps, 250);
    });

    test('is exactly the length the firmware expects', () {
      expect(PasParams().encode().length, 35);
      expect(PasParams.length, 35);
    });

    test('rejects an unknown version rather than mis-parsing it', () {
      final blob = PasParams().encode();
      blob[0] = 99; // a future firmware
      expect(PasParams.decode(blob), isNull);
    });

    test('rejects a short blob', () {
      expect(PasParams.decode(Uint8List(10)), isNull);
    });

    test('survives the isolate port as base64', () {
      final p = PasParams()
        ..maxCurrentMa = 33000
        ..canKbps = 1000;
      final back = PasParams.fromB64(p.toB64())!;
      expect(back.maxCurrentMa, 33000);
      expect(back.canKbps, 1000);
    });
  });

  group('HelperStatus', () {
    Uint8List frame({
      int flags = 0,
      int rpmCenti = 0,
      int batt = 0xFF,
      int level = 0,
      int assistMa = 0,
      int btnMask = 0,
      int btnCount = 0,
    }) {
      final b = ByteData(HelperStatus.length);
      b.setUint8(0, HelperStatus.version);
      b.setUint8(1, flags);
      b.setInt16(2, rpmCenti, Endian.little);
      b.setUint8(4, batt);
      b.setUint8(5, level);
      b.setInt32(6, assistMa, Endian.little);
      b.setUint8(10, btnMask);
      b.setUint8(11, btnCount);
      return b.buffer.asUint8List();
    }

    test('scales rpm and assist current', () {
      final s = HelperStatus.decode(frame(rpmCenti: 7250, assistMa: 12500))!;
      expect(s.rpm, closeTo(72.5, 0.001));
      expect(s.assistA, closeTo(12.5, 0.001));
    });

    test('decodes the flag bits', () {
      final s = HelperStatus.decode(frame(flags: 0xFF))!;
      expect(s.cadenceBound, isTrue);
      expect(s.cadenceConnected, isTrue);
      expect(s.remoteBound, isTrue);
      expect(s.remoteConnected, isTrue);
      expect(s.throttleOn, isTrue);
      expect(s.vescLink, isTrue);
      expect(s.pasEnabled, isTrue);

      final off = HelperStatus.decode(frame(flags: 0))!;
      expect(off.cadenceBound, isFalse);
      expect(off.pasEnabled, isFalse);
    });

    test('0xFF battery means "the sensor did not say"', () {
      expect(HelperStatus.decode(frame(batt: 0xFF))!.battKnown, isFalse);
      expect(HelperStatus.decode(frame(batt: 80))!.battKnown, isTrue);
    });

    test('rejects a wrong version and a short frame', () {
      final bad = frame();
      bad[0] = 1;
      expect(HelperStatus.decode(bad), isNull);
      expect(HelperStatus.decode(Uint8List(4)), isNull);
    });

    test('crosses the port as a map', () {
      final s = HelperStatus.decode(
          frame(flags: 0x42, rpmCenti: 5000, level: 3, btnCount: 2))!;
      final back = HelperStatus.fromMap(s.toMap());
      expect(back.flags, s.flags);
      expect(back.rpm, s.rpm);
      expect(back.level, 3);
      expect(back.btnCount, 2);
    });
  });

  group('ScanHit', () {
    Uint8List raw(String name, {int what = kWhatButton, int rssi = -55}) {
      final n = name.codeUnits;
      final out = Uint8List(10 + n.length);
      out[0] = what;
      out[1] = 0; // addr type
      out.setRange(2, 8, [0x11, 0x22, 0x33, 0x44, 0x55, 0x66]);
      ByteData.sublistView(out).setInt8(8, rssi);
      out[9] = n.length;
      out.setRange(10, 10 + n.length, n);
      return out;
    }

    test('renders the address in the human order', () {
      // NimBLE stores it least-significant byte first.
      expect(ScanHit.decode(raw('KY-01'))!.mac, '66:55:44:33:22:11');
    });

    test('decodes name, rssi and kind', () {
      final h = ScanHit.decode(raw('CADENCE', what: kWhatCadence, rssi: -80))!;
      expect(h.name, 'CADENCE');
      expect(h.rssi, -80);
      expect(h.what, kWhatCadence);
    });

    test('tolerates an empty name and a truncated one', () {
      expect(ScanHit.decode(raw(''))!.name, '');
      final short = raw('ABCDEF').sublist(0, 12); // claims 6, carries 2
      expect(() => ScanHit.decode(short), returnsNormally);
    });

    test('the raw address survives the port verbatim — bind depends on it',
        () {
      final h = ScanHit.decode(raw('X'))!;
      final back = ScanHit.fromMap(h.toMap());
      expect(back.addr, h.addr);
      expect(back.mac, h.mac);
    });

    test('rejects a runt', () => expect(ScanHit.decode(Uint8List(4)), isNull));
  });

  group('ButtonBinding', () {
    test('encodes the SET payload the firmware expects', () {
      final b = ButtonBinding(3, 0, 0x123, Uint8List.fromList([0, 1]));
      final out = b.encodeSet();
      expect(out.length, 16);
      expect(out[0], HelperCmd.setBinding);
      expect(out[1], 3); // idx
      expect(out[2], 0); // std id
      expect(out[3], 2); // dlc
      expect(ByteData.sublistView(out).getUint32(4, Endian.little), 0x123);
      expect(out.sublist(8, 10), [0, 1]);
      // Unused data bytes are zero-padded, not left uninitialised.
      expect(out.sublist(10), everyElement(0));
    });

    test('clamps an over-long data field to 8 bytes', () {
      final b = ButtonBinding(
          0, 0, 1, Uint8List.fromList(List.filled(12, 0xAB)));
      expect(b.encodeSet()[3], 8);
    });

    test('decodes a GET reply', () {
      final d = Uint8List(16);
      d[0] = 0x89;
      d[1] = 5; // idx
      d[2] = 1; // extended
      d[3] = 3; // dlc
      ByteData.sublistView(d).setUint32(4, 0x18FF1234, Endian.little);
      d.setRange(8, 11, [0xDE, 0xAD, 0xBE]);
      final b = ButtonBinding.decode(d)!;
      expect(b.idx, 5);
      expect(b.ext, 1);
      expect(b.canId, 0x18FF1234);
      expect(b.data, [0xDE, 0xAD, 0xBE]);
    });

    test('crosses the port intact', () {
      final b = ButtonBinding(2, 1, 0x7FF, Uint8List.fromList([9, 8]));
      final back = ButtonBinding.fromMap(b.toMap());
      expect(back.idx, 2);
      expect(back.ext, 1);
      expect(back.canId, 0x7FF);
      expect(back.data, [9, 8]);
    });
  });

  group('hex helpers', () {
    test('round-trip', () {
      expect(hexBytes(bytesFromHex('00 01 FF')), '0001FF');
    });

    test('spaces are allowed, odd length is not', () {
      expect(bytesFromHex('0a0b'), [10, 11]);
      expect(() => bytesFromHex('ABC'), throwsFormatException);
      expect(() => bytesFromHex('ZZ'), throwsFormatException);
    });
  });
}
