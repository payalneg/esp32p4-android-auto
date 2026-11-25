// Checks what the USB flasher will do with a firmware file before you point
// the phone at a real head unit.
//
//   dart run tool/check_usb_image.dart                  # staged assets
//   dart run tool/check_usb_image.dart custom.bin       # arbitrary .bin
//
// Prints the image kind (merged vs application), the flash offset, the chip it
// targets and the silicon revision range. Exit code 1 when a staged
// head/application pair does not form a valid image.
// ignore_for_file: avoid_print
import 'dart:io';
import 'dart:typed_data';

import 'package:aa_bridge/firmware/esp_image.dart';

const _boards = <String>['waveshare', 'waveshare-rev3', 'jc4880'];
const _assetDir = 'assets/firmware';
const _headLength = 0x20000;

void _report(String name, EspImageInfo info, int length) {
  final chip =
      info.chip?.name ?? (info.chipId == null ? '?' : 'chip ${info.chipId}');
  final kind = switch (info.kind) {
    EspImageKind.merged => 'merged (write at 0x0)',
    EspImageKind.application =>
      'application (write at 0x${info.flashOffset.toRadixString(16)})',
    EspImageKind.unknown => 'INVALID',
  };
  print(name);
  print('  size   : ${(length / 1024).round()} KB');
  print('  kind   : $kind');
  print('  chip   : $chip');
  print('  revs   : ${info.revisionRange}');
  if (info.errorKey != null) print('  error  : ${info.errorKey}');
}

Future<void> main(List<String> args) async {
  if (args.isNotEmpty) {
    var bad = 0;
    for (final path in args) {
      final bytes = await File(path).readAsBytes();
      final info = inspectEspImage(bytes);
      _report(path, info, bytes.length);
      if (!info.isValid) bad++;
    }
    if (bad > 0) exitCode = 1;
    return;
  }

  var bad = 0;
  for (final board in _boards) {
    final appFile = File('$_assetDir/esp32p4_android_auto-$board.bin');
    if (!await appFile.exists()) {
      print('$board: no staged application image — run '
          'scripts/stage_firmware_asset.sh first');
      bad++;
      continue;
    }
    final app = await appFile.readAsBytes();
    final headFile = File('$_assetDir/esp32p4_android_auto-$board-head.bin');
    if (!await headFile.exists()) {
      print('$board: no staged recovery head — USB flashing would write the '
          'application only');
      _report('$board (app only)', inspectEspImage(app), app.length);
      continue;
    }
    final head = await headFile.readAsBytes();
    if (head.length != _headLength) {
      print('$board: head is ${head.length} bytes, expected $_headLength');
      bad++;
      continue;
    }
    final merged = Uint8List(head.length + app.length)
      ..setAll(0, head)
      ..setAll(head.length, app);
    final info = inspectEspImage(merged);
    _report('$board (head + app)', info, merged.length);
    if (!info.isValid) bad++;
    final appInfo = inspectEspImage(app);
    if (appInfo.chipId != info.chipId) {
      print('  ERROR: app chip ${appInfo.chipId} != '
          'bootloader chip ${info.chipId}');
      bad++;
    }
    if (appInfo.minRev != info.minRev || appInfo.maxRev != info.maxRev) {
      print('  ERROR: app revs ${appInfo.revisionRange} != '
          'bootloader revs ${info.revisionRange}');
      bad++;
    }
  }
  if (bad > 0) exitCode = 1;
}
