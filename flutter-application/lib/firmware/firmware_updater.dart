/// Uploads the bundled P4 firmware image to the head unit's HTTP OTA endpoint.
///
/// Assumes the phone is already on the same network as the head unit (its
/// SoftAP) and reaches it by mDNS hostname — no in-app WiFi join. The head
/// unit writes the image to its spare OTA partition, verifies it and reboots,
/// so a successful upload ends with the device coming back on the new version.
library;

import 'dart:async';
import 'dart:convert';
import 'dart:io';
import 'dart:typed_data';

import 'package:flutter/services.dart' show rootBundle;

import '../ble/ble_proxy.dart';

enum UpdatePhase { idle, uploading, verifying, done, error }

/// How the firmware image reaches the head unit.
enum OtaTransport {
  /// HTTP POST over WiFi to the head unit's SoftAP — fast, but the phone must
  /// be on the head unit's network.
  wifi,

  /// Streamed straight over the existing BLE link — slower, but needs no WiFi.
  ble,

  /// Written straight into the chip's flash through a USB cable. Needs no
  /// network at all and works even when the firmware doesn't boot.
  usb,
}

class UpdateState {
  const UpdateState(this.phase, {this.progress = 0, this.messageKey, this.args});
  final UpdatePhase phase;

  /// 0..1 upload fraction, meaningful only while [phase] == uploading.
  final double progress;

  /// i18n key for the status/error line (null = no line). The UI localises it
  /// via `t()` — this layer has no BuildContext, so it never holds final text.
  final String? messageKey;

  /// Substitutions for {placeholder}s in the localised string (e.g. {size},
  /// {host}, {err}).
  final Map<String, String>? args;
}

/// A bundled image resolved for direct flashing over USB.
class UsbFirmwareImage {
  const UsbFirmwareImage(this.bytes, this.offset, this.merged);

  final Uint8List bytes;

  /// Flash offset: 0 for a merged recovery image, the application partition
  /// offset (0x20000) for an application-only image.
  final int offset;

  /// True when [bytes] contains a bootloader + partition table + app.
  final bool merged;
}

class FirmwareUpdater {
  static const _versionAsset = 'assets/firmware/version.txt';

  /// Board model used when the head unit doesn't report one (older firmware)
  /// and the user hasn't picked one manually.
  static const defaultModel = 'waveshare';

  /// S3 support is paused; only P4 images are bundled.
  /// Boards the APK ships an image for. Slugs match the firmware's
  /// BOARD_MODEL_ID (main/board.h) and the staged asset names
  /// (scripts/stage_firmware_asset.sh). Drives the manual firmware picker.
  static const boards = <String>[
    'waveshare', 'waveshare-rev3', 'jc4880',
  ];

  /// Human-readable product name for a board slug (untranslated — these are
  /// product names). Returns the raw slug for anything unrecognised.
  static String displayName(String? model) {
    switch (model) {
      case 'waveshare':
        return 'Waveshare 4.3" (P4 v1.x)';
      case 'waveshare-rev3':
        return 'Waveshare 4.3" (P4 v3.x)';
      case 'jc4880':
        return 'Guition JC4880P443C';
      case 's3touch4':
        return 'Waveshare ESP32-S3 4" (480x480)';
      default:
        return model ?? '?';
    }
  }

  /// Resolve the bundled firmware asset for a given board model. The APK ships
  /// one image per board/revision. Legacy firmware without a model uses the
  /// default; an explicit unknown model must never silently select a v1 image.
  static String assetFor(String? model) {
    final m = model ?? defaultModel;
    if (!boards.contains(m)) {
      throw ArgumentError.value(model, 'model', 'Unsupported firmware profile');
    }
    return 'assets/firmware/esp32p4_android_auto-$m.bin';
  }

  /// A detected profile cannot be overridden with an incompatible image.
  /// Without device info the user must explicitly choose a known profile.
  static bool canFlash(String? detected, String? selected) =>
      boards.contains(selected) && (detected == null || detected == selected);

  /// Load the bundled firmware image bytes for [model]. Usable from the
  /// background isolate (rootBundle works there too) — the BLE OTA flash now
  /// runs in the task isolate that owns the link, so it loads the image itself.
  static Future<Uint8List> imageFor(String? model) async {
    final data = await rootBundle.load(assetFor(model));
    return data.buffer.asUint8List(data.offsetInBytes, data.lengthInBytes);
  }

  /// The first 0x20000 bytes of a merged image: 0xFF padding up to the
  /// bootloader at 0x2000 and the partition table at 0x8000, with 0xFF up to
  /// the application partition. Staged next to the application .bin by
  /// scripts/stage_firmware_asset.sh; combining it with that .bin at flash
  /// time gives a full recovery image without shipping a second 4.8 MB copy
  /// per board in the APK.
  static const mergedHeadLength = 0x20000;

  static String headAssetFor(String? model) =>
      'assets/firmware/esp32p4_android_auto-${model ?? defaultModel}-head.bin';

  /// Whether this APK was built with the recovery-image head for [model].
  static Future<bool> hasMergedImage(String? model) async {
    try {
      final head = await rootBundle.load(headAssetFor(model));
      return head.lengthInBytes == mergedHeadLength &&
          head.getUint8(0x2000) == 0xE9;
    } catch (_) {
      return false;
    }
  }

  /// Bundled image ready for direct (USB) flashing. Prefers a full recovery
  /// image; falls back to the application-only OTA image when the APK was
  /// staged without the merged head.
  static Future<UsbFirmwareImage> usbImageFor(String? model) async {
    final app = await imageFor(model);
    try {
      final head = await rootBundle.load(headAssetFor(model));
      if (head.lengthInBytes == mergedHeadLength && head.getUint8(0x2000) == 0xE9) {
        final out = Uint8List(mergedHeadLength + app.length);
        out.setAll(
            0,
            head.buffer.asUint8List(
                head.offsetInBytes, head.lengthInBytes));
        out.setAll(mergedHeadLength, app);
        return UsbFirmwareImage(out, 0, true);
      }
    } catch (_) {
      // Not staged: still allow the application-only write.
    }
    return UsbFirmwareImage(app, 0x20000, false);
  }

  /// Whether this board runs the Android Auto side of the firmware. The
  /// ESP32-S3 board is the VESC dashboard alone (no AA), which changes what
  /// the app offers for it — see [hostFor] and the boot-splash builder.
  static bool hasAndroidAuto(String? model) => model != 's3touch4';

  /// Default OTA host — the head unit's mDNS name (CONFIG_WEB_MDNS_HOSTNAME +
  /// .local). The dashboard-only board answers to a different name because it
  /// is not an "android-auto" host.
  static String hostFor(String? model) =>
      hasAndroidAuto(model) ? 'android-auto.local' : 'vesc-display.local';

  /// Default OTA host for a head unit whose model we don't know yet.
  static const defaultHost = 'android-auto.local';

  /// Version string of the firmware image bundled in this APK. The version is
  /// shared across boards, so it's board-agnostic.
  static Future<String> bundledVersion() async =>
      (await rootBundle.loadString(_versionAsset)).trim();

  /// Compare dotted numeric versions: `1.3.10` is newer than `1.3.9`, and a
  /// missing component counts as zero (`1.3` == `1.3.0`). Anything after the
  /// numeric parts (a `-rc1` suffix, build metadata) is ignored — the release
  /// pipeline only uses plain `x.y.z`.
  static int compareVersions(String a, String b) {
    List<int> parts(String v) {
      // Take only the leading dotted numeric portion: a "-rc1" suffix or
      // build metadata must not count as an extra version component.
      final m = RegExp(r'^\s*v?(\d+(?:\.\d+)*)').firstMatch(v);
      if (m == null) return const <int>[];
      return m
          .group(1)!
          .split('.')
          .map((s) => int.tryParse(s) ?? 0)
          .toList(growable: false);
    }

    final pa = parts(a);
    final pb = parts(b);
    final n = pa.length > pb.length ? pa.length : pb.length;
    for (var i = 0; i < n; i++) {
      final va = i < pa.length ? pa[i] : 0;
      final vb = i < pb.length ? pb[i] : 0;
      if (va != vb) return va < vb ? -1 : 1;
    }
    return 0;
  }

  /// SharedPreferences key remembering a version the rider skipped on
  /// [model]. Per board, so skipping a release on one head unit does not hide
  /// it on another.
  static String skippedVersionKey(String model) => 'fw_skip_v1_$model';

  final _ctrl = StreamController<UpdateState>.broadcast();
  Stream<UpdateState> get state => _ctrl.stream;

  void _emit(UpdatePhase p,
      {double progress = 0, String? messageKey, Map<String, String>? args}) {
    // The update screen closes this controller in dispose(); if the user
    // navigates away / backgrounds the app mid-flash, the in-flight transfer
    // keeps running (foreground service) and its progress callbacks must not
    // crash on a closed stream — that "Cannot add new events after calling
    // close" exception was aborting the OTA. Drop late emits silently; the
    // transfer still completes and reboots the head unit.
    if (_ctrl.isClosed) return;
    _ctrl.add(UpdateState(p,
        progress: progress, messageKey: messageKey, args: args));
  }

  /// POST the bundled image to http://[host]/ota. [model] selects which board's
  /// image to send (from the head unit's reported board model); null falls back
  /// to the default board. Never throws — failures surface through the [state]
  /// stream and the bool result.
  Future<bool> run({String host = defaultHost, String? model}) async {
    final url = 'http://$host/ota';
    try {
      final bytes = (await rootBundle.load(assetFor(model))).buffer.asUint8List();
      _emit(UpdatePhase.uploading, progress: 0,
          messageKey: 'fw.ota.uploading.wifi',
          args: {'size': (bytes.length / 1024).round().toString()});
      final ok = await _post(url, bytes);
      if (!ok) {
        _emit(UpdatePhase.error, messageKey: 'fw.ota.rejected');
        return false;
      }
      _emit(UpdatePhase.done, messageKey: 'fw.done');
      return true;
    } catch (e) {
      _emit(UpdatePhase.error,
          messageKey: 'fw.ota.connfail', args: {'host': host, 'err': '$e'});
      return false;
    }
  }

  /// Flash the bundled image over the BLE link (no WiFi). [model] selects the
  /// board image; null falls back to the default board. Computes the image's
  /// SHA-256 so the head unit can verify the transfer, then streams it via
  /// [BleService.bleOta]. Never throws — failures surface through [state] and
  /// the bool result.
  Future<bool> runBle({String? model}) async {
    try {
      _emit(UpdatePhase.uploading, progress: 0,
          messageKey: 'fw.ota.uploading.ble');
      // The flash runs in the background isolate that owns the BLE link: it
      // loads the bundled image, hashes it and streams it. We just relay its
      // progress / verify / result over the port.
      final res = await BleProxy.instance.bleOta(
        model,
        onUpload: (f) => _emit(UpdatePhase.uploading, progress: f),
        onVerify: () => _emit(UpdatePhase.verifying,
            progress: 1, messageKey: 'fw.ota.verifying.device'),
      );
      if (!res.ok) {
        _emit(UpdatePhase.error, messageKey: res.errorKey ?? 'fw.ota.blefail');
        return false;
      }
      _emit(UpdatePhase.done, messageKey: 'fw.done');
      return true;
    } catch (e) {
      _emit(UpdatePhase.error, messageKey: 'fw.ota.bleerror', args: {'err': '$e'});
      return false;
    }
  }

  Future<bool> _post(String url, List<int> bytes) async {
    final client = HttpClient()
      ..connectionTimeout = const Duration(seconds: 15);
    try {
      final req = await client.postUrl(Uri.parse(url));
      req.headers.contentType = ContentType('application', 'octet-stream');
      req.contentLength = bytes.length;

      const chunk = 16 * 1024;
      var sent = 0;
      while (sent < bytes.length) {
        final end = (sent + chunk) < bytes.length ? sent + chunk : bytes.length;
        req.add(bytes.sublist(sent, end));
        await req.flush();
        sent = end;
        _emit(UpdatePhase.uploading, progress: sent / bytes.length);
      }
      // Device verifies + commits after the body lands.
      _emit(UpdatePhase.verifying, progress: 1, messageKey: 'fw.ota.verifying');
      final resp = await req.close().timeout(const Duration(seconds: 60));
      final body = await resp.transform(utf8.decoder).join();
      return resp.statusCode == 200 &&
          body.toLowerCase().contains('rebooting');
    } finally {
      client.close(force: true);
    }
  }

  void dispose() => _ctrl.close();
}
