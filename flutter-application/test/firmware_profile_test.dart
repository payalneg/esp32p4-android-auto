import 'dart:convert';

import 'package:aa_bridge/firmware/firmware_updater.dart';
import 'package:aa_bridge/firmware/ota_info.dart';
import 'package:flutter_test/flutter_test.dart';

void main() {
  test(
    'P4 v3 profile survives BLE discovery and resolves its own OTA asset',
    () {
      final info = OtaInfo.parse(
        utf8.encode(
          '192.168.4.1\n80\nHeadUnit\npassword\n1.3.33\nwaveshare-rev3',
        ),
      )!;
      expect(info.model, 'waveshare-rev3');
      expect(
        FirmwareUpdater.assetFor(info.model),
        'assets/firmware/esp32p4_android_auto-waveshare-rev3.bin',
      );
      expect(FirmwareUpdater.canFlash(info.model, info.model), isTrue);
      expect(FirmwareUpdater.hasAndroidAuto(info.model), isTrue);
    },
  );

  test('P4 revisions cannot be interchanged in the picker', () {
    expect(FirmwareUpdater.canFlash('waveshare-rev3', 'waveshare'), isFalse);
    expect(FirmwareUpdater.canFlash('waveshare', 'waveshare-rev3'), isFalse);
    expect(
      FirmwareUpdater.assetFor('waveshare'),
      'assets/firmware/esp32p4_android_auto-waveshare.bin',
    );
  });

  test('unknown explicit profile cannot silently select a v1 image', () {
    expect(
      () => FirmwareUpdater.assetFor('waveshare-rev4'),
      throwsArgumentError,
    );
    expect(FirmwareUpdater.canFlash('waveshare-rev4', 'waveshare'), isFalse);
  });

  test('legacy discovery needs a manual selection', () {
    final info = OtaInfo.parse(
      utf8.encode('192.168.4.1\n80\nHeadUnit\npassword\n1.0.0'),
    )!;
    expect(info.model, isNull);
    expect(FirmwareUpdater.canFlash(info.model, null), isFalse);
    expect(FirmwareUpdater.canFlash(info.model, 'waveshare-rev3'), isTrue);
  });
}
