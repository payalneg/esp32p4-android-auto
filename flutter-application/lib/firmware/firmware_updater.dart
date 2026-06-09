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

import 'package:crypto/crypto.dart' show sha256;
import 'package:flutter/services.dart' show rootBundle;

import '../ble/ble_service.dart';

enum UpdatePhase { idle, uploading, verifying, done, error }

/// How the firmware image reaches the head unit.
enum OtaTransport {
  /// HTTP POST over WiFi to the head unit's SoftAP — fast, but the phone must
  /// be on the head unit's network.
  wifi,

  /// Streamed straight over the existing BLE link — slower, but needs no WiFi.
  ble,
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

class FirmwareUpdater {
  static const _versionAsset = 'assets/firmware/version.txt';

  /// Board model used when the head unit doesn't report one (older firmware)
  /// and the user hasn't picked one manually.
  static const defaultModel = 'waveshare';

  /// Boards the APK ships an image for. Slugs match the firmware's
  /// BOARD_MODEL_ID (main/board.h) and the staged asset names
  /// (scripts/stage_firmware_asset.sh). Drives the manual firmware picker.
  static const boards = <String>['waveshare', 'jc4880'];

  /// Human-readable product name for a board slug (untranslated — these are
  /// product names). Returns the raw slug for anything unrecognised.
  static String displayName(String? model) {
    switch (model) {
      case 'waveshare':
        return 'Waveshare 4.3"';
      case 'jc4880':
        return 'Guition JC4880P443C';
      default:
        return model ?? '?';
    }
  }

  /// Resolve the bundled firmware asset for a given board model. The APK ships
  /// one image per board; falls back to the default board when [model] is null
  /// or unrecognised.
  static String _assetFor(String? model) {
    final m = boards.contains(model) ? model! : defaultModel;
    return 'assets/firmware/esp32p4_android_auto-$m.bin';
  }

  /// Default OTA host — the head unit's mDNS name (AA_MDNS_HOSTNAME + .local).
  static const defaultHost = 'android-auto.local';

  /// Version string of the firmware image bundled in this APK. The version is
  /// shared across boards, so it's board-agnostic.
  static Future<String> bundledVersion() async =>
      (await rootBundle.loadString(_versionAsset)).trim();

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
      final bytes = (await rootBundle.load(_assetFor(model))).buffer.asUint8List();
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
      final bytes = (await rootBundle.load(_assetFor(model))).buffer.asUint8List();
      _emit(UpdatePhase.uploading, progress: 0,
          messageKey: 'fw.ota.uploading.ble',
          args: {'size': (bytes.length / 1024).round().toString()});
      // Hashing ~4 MB is quick but blocks the isolate; nudge it off the
      // current microtask so the initial frame paints first.
      final digest = await Future(
          () => Uint8List.fromList(sha256.convert(bytes).bytes));
      final res = await BleService.instance.bleOta(
        bytes,
        digest,
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
