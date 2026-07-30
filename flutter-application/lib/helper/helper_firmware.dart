/// Where the VESC BLE Helper firmware comes from.
///
/// Two sources, in this order:
///
///  1. **Bundled in the APK** — staged at build time by
///     `scripts/stage_firmware_asset.sh`, which pulls it from the helper
///     project's GitHub releases. This is the one that matters: flashing a
///     helper happens in a garage, which is exactly where there is no network.
///  2. **GitHub, on demand** — the "check" button, for when the helper project
///     has released something newer than the bundle.
///
/// Uses `dart:io HttpClient` directly, like the firmware OTA uploader — no new
/// dependency, and it gives per-chunk progress for free.
library;

import 'dart:async';
import 'dart:convert';
import 'dart:io';
import 'dart:typed_data';

import 'package:flutter/services.dart' show rootBundle;

const kHelperRepo = 'payalneg/esp32c3-ble-helper';

/// The asset we want is the plain application image, e.g.
/// `esp32c3_ble_helper-1.0.4.bin`.
///
/// The version group is digits-and-dots ONLY, and that is load-bearing: the
/// same release also ships `esp32c3_ble_helper-1.0.4-merged.bin`, a full flash
/// image (bootloader + partition table + app) for esptool. Flashing that as an
/// OTA update would brick the helper — and it happens to be listed first, so a
/// looser pattern picks exactly the wrong file.
final _assetRe = RegExp(r'^esp32c3_ble_helper-([0-9][0-9.]*)\.bin$');

class HelperFirmwareException implements Exception {
  const HelperFirmwareException(this.message);
  final String message;
  @override
  String toString() => message;
}

class HelperRelease {
  const HelperRelease({
    required this.version,
    required this.tag,
    required this.url,
    required this.sizeBytes,
  });

  /// Version as it appears in the asset name (e.g. `1.0.4`) — directly
  /// comparable with what the helper reports over its Device Information
  /// Service.
  final String version;
  final String tag;
  final String url;
  final int sizeBytes;
}

/// The firmware bundled with this build, if there is one.
class BundledHelperFirmware {
  const BundledHelperFirmware(this.version, this.bytes);
  final String version;
  final Uint8List bytes;
  int get sizeBytes => bytes.length;
}

class HelperFirmware {
  HelperFirmware({HttpClient Function()? httpClientFactory})
      : _newClient = httpClientFactory ?? HttpClient.new;

  final HttpClient Function() _newClient;

  static const _imageAsset = 'assets/firmware/esp32c3_ble_helper.bin';
  static const _versionAsset =
      'assets/firmware/esp32c3_ble_helper_version.txt';

  /// Read the bundled image. Null when this build shipped without one (the
  /// staging script writes empty placeholders if it couldn't reach GitHub).
  Future<BundledHelperFirmware?> loadBundled() async {
    try {
      final version = (await rootBundle.loadString(_versionAsset)).trim();
      final data = await rootBundle.load(_imageAsset);
      final bytes = data.buffer.asUint8List();
      if (version.isEmpty || bytes.isEmpty) return null;
      return BundledHelperFirmware(version, bytes);
    } catch (_) {
      return null;
    }
  }

  /// Ask GitHub for the newest release and pick the OTA image out of it.
  ///
  /// Anonymous API calls are rate-limited to 60/hour per IP, which is far more
  /// than a settings screen needs; a 403 from that limit surfaces as a plain
  /// error rather than a retry storm.
  Future<HelperRelease> fetchLatest() async {
    final client = _newClient()
      ..connectionTimeout = const Duration(seconds: 15);
    try {
      final req = await client.getUrl(
          Uri.parse('https://api.github.com/repos/$kHelperRepo/releases/latest'));
      req.headers
        ..set(HttpHeaders.acceptHeader, 'application/vnd.github+json')
        ..set(HttpHeaders.userAgentHeader, 'aa-bridge-app');
      final resp = await req.close().timeout(const Duration(seconds: 30));
      final body = await resp.transform(utf8.decoder).join();
      if (resp.statusCode != 200) {
        throw HelperFirmwareException('GitHub returned ${resp.statusCode}');
      }

      final json = jsonDecode(body) as Map<String, dynamic>;
      final tag = json['tag_name'] as String? ?? '';
      for (final raw in (json['assets'] as List?) ?? const []) {
        final a = (raw as Map).cast<String, dynamic>();
        final name = a['name'] as String? ?? '';
        final m = _assetRe.firstMatch(name);
        if (m == null) continue;
        return HelperRelease(
          version: m.group(1)!,
          tag: tag,
          url: a['browser_download_url'] as String? ?? '',
          sizeBytes: (a['size'] as num?)?.toInt() ?? 0,
        );
      }
      throw const HelperFirmwareException(
          'no firmware image in the latest release');
    } on SocketException catch (e) {
      throw HelperFirmwareException(e.message);
    } on HandshakeException catch (e) {
      throw HelperFirmwareException(e.message);
    } finally {
      client.close(force: true);
    }
  }

  /// Download the image, reporting progress 0..1 when the server states a
  /// content length (GitHub's asset CDN does).
  Future<Uint8List> download(HelperRelease release,
      {void Function(double)? onProgress}) async {
    final client = _newClient()
      ..connectionTimeout = const Duration(seconds: 15);
    try {
      var url = Uri.parse(release.url);
      HttpClientResponse resp;
      // The asset URL redirects to the CDN; follow a bounded chain by hand so
      // a redirect loop can't hang the screen.
      for (var hop = 0;; hop++) {
        final req = await client.getUrl(url);
        req
          ..followRedirects = false
          ..headers.set(HttpHeaders.userAgentHeader, 'aa-bridge-app');
        resp = await req.close().timeout(const Duration(seconds: 60));
        final location = resp.headers.value(HttpHeaders.locationHeader);
        if (resp.isRedirect && location != null && hop < 5) {
          await resp.drain<void>();
          url = url.resolve(location);
          continue;
        }
        break;
      }
      if (resp.statusCode != 200) {
        throw HelperFirmwareException('download failed: ${resp.statusCode}');
      }

      final total = resp.contentLength > 0
          ? resp.contentLength
          : release.sizeBytes;
      final out = BytesBuilder(copy: false);
      await for (final chunk in resp) {
        out.add(chunk);
        if (total > 0) onProgress?.call((out.length / total).clamp(0, 1));
      }
      final bytes = out.takeBytes();
      if (bytes.isEmpty) {
        throw const HelperFirmwareException('downloaded an empty image');
      }
      // A truncated download would be flashed and brick the helper, so refuse
      // anything that doesn't match the size the release advertised.
      if (release.sizeBytes > 0 && bytes.length != release.sizeBytes) {
        throw HelperFirmwareException(
            'size mismatch: got ${bytes.length} B, expected '
            '${release.sizeBytes} B');
      }
      return bytes;
    } on SocketException catch (e) {
      throw HelperFirmwareException(e.message);
    } finally {
      client.close(force: true);
    }
  }
}
