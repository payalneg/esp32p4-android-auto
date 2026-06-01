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

import 'package:flutter/services.dart' show rootBundle;

enum UpdatePhase { idle, uploading, verifying, done, error }

class UpdateState {
  const UpdateState(this.phase, {this.progress = 0, this.message});
  final UpdatePhase phase;

  /// 0..1 upload fraction, meaningful only while [phase] == uploading.
  final double progress;
  final String? message;
}

class FirmwareUpdater {
  static const _asset = 'assets/firmware/esp32p4_android_auto.bin';
  static const _versionAsset = 'assets/firmware/version.txt';

  /// Default OTA host — the head unit's mDNS name (AA_MDNS_HOSTNAME + .local).
  static const defaultHost = 'android-auto.local';

  /// Version string of the firmware image bundled in this APK.
  static Future<String> bundledVersion() async =>
      (await rootBundle.loadString(_versionAsset)).trim();

  final _ctrl = StreamController<UpdateState>.broadcast();
  Stream<UpdateState> get state => _ctrl.stream;

  void _emit(UpdatePhase p, {double progress = 0, String? message}) =>
      _ctrl.add(UpdateState(p, progress: progress, message: message));

  /// POST the bundled image to http://[host]/ota. Never throws — failures
  /// surface through the [state] stream and the bool result.
  Future<bool> run({String host = defaultHost}) async {
    final url = 'http://$host/ota';
    try {
      final bytes = (await rootBundle.load(_asset)).buffer.asUint8List();
      _emit(UpdatePhase.uploading, progress: 0,
          message: 'Загрузка прошивки (${(bytes.length / 1024).round()} КБ)…');
      final ok = await _post(url, bytes);
      if (!ok) {
        _emit(UpdatePhase.error, message: 'Устройство отклонило прошивку');
        return false;
      }
      _emit(UpdatePhase.done, message: 'Прошивка принята — устройство перезагружается');
      return true;
    } catch (e) {
      _emit(UpdatePhase.error, message: 'Не удалось подключиться к $host: $e');
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
      _emit(UpdatePhase.verifying, progress: 1, message: 'Проверка образа…');
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
