import 'dart:typed_data';

import 'package:aa_bridge/firmware/esp_image.dart';
import 'package:aa_bridge/firmware/esptool/esp_chip.dart';
import 'package:aa_bridge/firmware/esptool/esp_loader.dart';
import 'package:aa_bridge/firmware/esptool/slip.dart';
import 'package:aa_bridge/firmware/firmware_updater.dart';
import 'package:flutter_test/flutter_test.dart';

/// Builds a synthetic ESP image header + (empty) segment area.
Uint8List _image({
  int chipId = 18,
  int minRev = 100,
  int maxRev = 199,
  int length = 0x10000,
}) {
  final data = Uint8List(length)..fillRange(0, length, 0x00);
  data[0] = 0xE9;
  data[1] = 0x01; // one segment
  ByteData.sublistView(data, 0, 24)
    ..setUint16(12, chipId, Endian.little)
    ..setUint8(14, 0)
    ..setUint16(15, minRev, Endian.little)
    ..setUint16(17, maxRev, Endian.little);
  return data;
}

/// A P4 merged image: 0xFF padding, bootloader at 0x2000, a valid partition
/// table at 0x8000 and the application at 0x20000.
Uint8List _mergedP4({int minRev = 100, int maxRev = 199}) {
  final data = Uint8List(0x20000 + 0x1000)..fillRange(0, 0x21000, 0xFF);
  final boot = _image(minRev: minRev, maxRev: maxRev, length: 0x6000);
  data.setAll(0x2000, boot);
  // Two partition table entries: magic, type, subtype, offset, size, label.
  for (var i = 0; i < 2; i++) {
    final off = 0x8000 + i * 32;
    data[off] = 0xAA;
    data[off + 1] = 0x50;
    data[off + 2] = 0x00;
    data[off + 3] = i == 0 ? 0x00 : 0x10;
    ByteData.sublistView(data, off + 4, off + 12)
      ..setUint32(0, i == 0 ? 0x20000 : 0x9000, Endian.little)
      ..setUint32(4, 0x10000, Endian.little);
  }
  final app = _image(minRev: minRev, maxRev: maxRev, length: 0x1000);
  data.setAll(0x20000, app);
  return data;
}

void main() {
  group('SLIP', () {
    test('round-trips escaped bytes', () {
      final payload = <int>[
        0x00,
        0xC0,
        0xDB,
        0xDB,
        0xC0,
        0x01,
        0xFF,
        ...List<int>.generate(300, (i) => i & 0xFF),
      ];
      final frames = SlipDecoder().add(slipEncode(payload));
      expect(frames, hasLength(1));
      expect(frames.single, payload);
    });

    test('decodes a frame split across chunks', () {
      final payload = <int>[1, 2, 0xC0, 3, 0xDB, 4];
      final encoded = slipEncode(payload);
      final decoder = SlipDecoder();
      final frames = <Uint8List>[];
      for (var i = 0; i < encoded.length; i += 2) {
        frames.addAll(decoder.add(encoded.sublist(
            i, i + 2 > encoded.length ? encoded.length : i + 2)));
      }
      expect(frames, hasLength(1));
      expect(frames.single, payload);
    });

    test('ignores empty frames and resets a partial packet', () {
      final decoder = SlipDecoder();
      expect(decoder.add(<int>[0xC0, 0xC0]), isEmpty);
      expect(decoder.add(<int>[1, 2, 3]), isEmpty);
      decoder.reset();
      expect(decoder.add(<int>[0xC0]), isEmpty,
          reason: 'the partial frame was dropped');
      final frames = decoder.add(<int>[5, 0xC0]);
      expect(frames, hasLength(1));
      expect(frames.single, <int>[5]);
    });
  });

  group('ESP image inspection', () {
    test('P4 merged image is written at 0x0', () {
      final info = inspectEspImage(_mergedP4());
      expect(info.isValid, isTrue);
      expect(info.kind, EspImageKind.merged);
      expect(info.flashOffset, 0);
      expect(info.bootloaderOffset, 0x2000);
      expect(info.chip, EspChip.esp32p4);
      expect(info.revisionRange, 'v1.0–v1.99');
      expect(info.acceptsRevision(100), isTrue);
      expect(info.acceptsRevision(301), isFalse);
    });

    test('rev3 merged image rejects a v1 chip and accepts v3.x', () {
      final info = inspectEspImage(_mergedP4(minRev: 300, maxRev: 399));
      expect(info.isValid, isTrue);
      expect(info.revisionRange, 'v3.0–v3.99');
      expect(info.acceptsRevision(100), isFalse);
      expect(info.acceptsRevision(301), isTrue);
    });

    test('application image needs an app-partition offset', () {
      final info = inspectEspImage(_image());
      expect(info.isValid, isTrue);
      expect(info.kind, EspImageKind.application);
      expect(info.flashOffset, 0x20000);
      expect(info.chipId, 18);
      expect(info.acceptsRevision(100), isTrue);
      expect(info.acceptsRevision(300), isFalse);
    });

    test('non-P4 application images default to the IDF 0x10000 slot', () {
      final info = inspectEspImage(_image(chipId: 9, minRev: 0, maxRev: 99));
      expect(info.kind, EspImageKind.application);
      expect(info.chip, EspChip.esp32s3);
      expect(info.flashOffset, 0x10000);
    });

    test('random data is rejected', () {
      final info = inspectEspImage(Uint8List.fromList([1, 2, 3, 4]));
      expect(info.isValid, isFalse);
      expect(info.errorKey, 'fw.usb.file.invalid');
    });

    test('a P4 bootloader without a partition table is not merged', () {
      final data = Uint8List(0x4000)..fillRange(0, 0x4000, 0xFF);
      data.setAll(0x2000, _image(length: 0x1000));
      final info = inspectEspImage(data);
      expect(info.isValid, isFalse);
    });

    test('a merged head without the application is rejected', () {
      final head = Uint8List.fromList(_mergedP4().sublist(0, 0x20000));
      final info = inspectEspImage(head);
      expect(info.isValid, isFalse);
      expect(info.errorKey, 'fw.usb.file.invalid');
    });

    test('a merged image whose app targets another chip is rejected', () {
      final data = _mergedP4();
      ByteData.sublistView(data, 0x20000 + 12, 0x20000 + 14)
          .setUint16(0, 9, Endian.little); // app says ESP32-S3
      final info = inspectEspImage(data);
      expect(info.isValid, isFalse);
    });
  });

  group('chips', () {
    test('ids map to names and bootloader offsets', () {
      expect(EspChip.byId(18)?.name, 'ESP32-P4');
      expect(EspChip.byId(9)?.name, 'ESP32-S3');
      expect(EspChip.byId(5)?.name, 'ESP32-C3');
      expect(EspChip.byId(0)?.name, 'ESP32');
      expect(EspChip.byId(99), isNull);
      expect(EspChip.esp32p4.bootloaderOffset, 0x2000);
      expect(EspChip.esp32p4.encryptedFlash, isTrue);
      expect(EspChip.esp32.encryptedFlash, isFalse);
      expect(EspChip.revString(301), 'v3.1');
      expect(EspChip.revString(100), 'v1.0');
    });
  });

  group('ROM helpers', () {
    test('checksum is the byte-wise XOR seeded with 0xEF', () {
      expect(EspLoader.checksum(const <int>[]), 0xEF);
      expect(EspLoader.checksum(const <int>[0x00]), 0xEF);
      expect(EspLoader.checksum(const <int>[0xEF]), 0x00);
      expect(EspLoader.checksum(const <int>[0x01, 0x02]), 0xEC);
    });
  });

  group('bundled USB image assets', () {
    test('head asset names match the board slugs', () {
      expect(
        FirmwareUpdater.headAssetFor('waveshare'),
        'assets/firmware/esp32p4_android_auto-waveshare-head.bin',
      );
      expect(
        FirmwareUpdater.headAssetFor('waveshare-rev3'),
        'assets/firmware/esp32p4_android_auto-waveshare-rev3-head.bin',
      );
      expect(FirmwareUpdater.headAssetFor(null),
          'assets/firmware/esp32p4_android_auto-waveshare-head.bin');
      expect(FirmwareUpdater.mergedHeadLength, 0x20000);
    });
  });
}
