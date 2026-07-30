/// Release-asset picking for the helper firmware download.
///
/// The one that matters: a GitHub release carries BOTH
/// `esp32c3_ble_helper-<v>.bin` (the OTA image) and
/// `esp32c3_ble_helper-<v>-merged.bin` (a full esptool flash image), and the
/// merged one is listed FIRST. Picking it would brick the helper.
library;

import 'dart:async';
import 'dart:convert';
import 'dart:io';

import 'package:aa_bridge/helper/helper_firmware.dart';
import 'package:flutter_test/flutter_test.dart';

/// Minimal fake of the GitHub API + asset CDN.
class _FakeGitHub {
  _FakeGitHub({required this.releaseJson, this.imageBytes = const []});

  final String releaseJson;
  final List<int> imageBytes;
  late HttpServer server;

  Future<void> start() async {
    server = await HttpServer.bind(InternetAddress.loopbackIPv4, 0);
    unawaited(server.forEach((req) async {
      if (req.uri.path.endsWith('/releases/latest')) {
        req.response
          ..statusCode = 200
          ..headers.contentType = ContentType.json
          ..write(releaseJson);
      } else if (req.uri.path.contains('redirect')) {
        req.response
          ..statusCode = 302
          ..headers.set(HttpHeaders.locationHeader, '$origin/asset.bin');
      } else if (req.uri.path.endsWith('/asset.bin')) {
        req.response
          ..statusCode = 200
          ..add(imageBytes);
      } else {
        req.response.statusCode = 404;
      }
      await req.response.close();
    }));
  }

  String get origin => 'http://127.0.0.1:${server.port}';
  Future<void> stop() => server.close(force: true);
}

String releaseWith(List<(String, int, String)> assets, {String tag = 'v1.0.4'}) =>
    jsonEncode({
      'tag_name': tag,
      'assets': [
        for (final (name, size, url) in assets)
          {'name': name, 'size': size, 'browser_download_url': url},
      ],
    });

void main() {
  group('asset picking', () {
    late _FakeGitHub gh;

    tearDown(() => gh.stop());

    Future<HelperRelease> fetchFrom(String json) async {
      gh = _FakeGitHub(releaseJson: json);
      await gh.start();
      // Point the client at the fake by overriding the host resolution: the
      // production URL is fixed, so we exercise the parser through a client
      // whose requests all land on the fake server.
      final fw = HelperFirmware(
          httpClientFactory: () => _RedirectingClient(gh.origin));
      return fw.fetchLatest();
    }

    test('skips the merged image even though it is listed first', () async {
      final r = await fetchFrom(releaseWith([
        ('esp32c3_ble_helper-1.0.4-merged.bin', 764176, 'http://x/merged.bin'),
        ('esp32c3_ble_helper-1.0.4.bin', 633104, 'http://x/app.bin'),
        ('vesc_ble_helper-1.0.4.apk', 21718238, 'http://x/app.apk'),
      ]));
      expect(r.version, '1.0.4');
      expect(r.sizeBytes, 633104);
      expect(r.url, 'http://x/app.bin');
      expect(r.tag, 'v1.0.4');
    });

    test('ignores the APK and other assets', () async {
      final r = await fetchFrom(releaseWith([
        ('vesc_ble_helper-9.9.9.apk', 1, 'http://x/a.apk'),
        ('README.md', 1, 'http://x/r.md'),
        ('esp32c3_ble_helper-2.1.0.bin', 42, 'http://x/app.bin'),
      ]));
      expect(r.version, '2.1.0');
    });

    test('a release with no OTA image is an error, not a wrong pick',
        () async {
      gh = _FakeGitHub(
          releaseJson: releaseWith([
        ('esp32c3_ble_helper-1.0.4-merged.bin', 1, 'http://x/m.bin'),
      ]));
      await gh.start();
      final fw =
          HelperFirmware(httpClientFactory: () => _RedirectingClient(gh.origin));
      expect(fw.fetchLatest(), throwsA(isA<HelperFirmwareException>()));
    });
  });

  group('download', () {
    test('follows the CDN redirect and checks the size', () async {
      final bytes = List<int>.generate(2048, (i) => i % 256);
      final gh = _FakeGitHub(releaseJson: '{}', imageBytes: bytes);
      await gh.start();
      addTearDown(gh.stop);

      final fw = HelperFirmware();
      final got = await fw.download(HelperRelease(
        version: '1.0.4',
        tag: 'v1.0.4',
        url: '${gh.origin}/redirect',
        sizeBytes: bytes.length,
      ));
      expect(got, bytes);
    });

    test('refuses a truncated image rather than flashing it', () async {
      final gh = _FakeGitHub(releaseJson: '{}', imageBytes: List.filled(10, 0));
      await gh.start();
      addTearDown(gh.stop);

      final fw = HelperFirmware();
      expect(
        fw.download(HelperRelease(
          version: '1.0.4',
          tag: 'v1.0.4',
          url: '${gh.origin}/asset.bin',
          sizeBytes: 999, // release says it should be bigger
        )),
        throwsA(isA<HelperFirmwareException>()),
      );
    });

    test('refuses an empty body', () async {
      final gh = _FakeGitHub(releaseJson: '{}');
      await gh.start();
      addTearDown(gh.stop);

      final fw = HelperFirmware();
      expect(
        fw.download(HelperRelease(
            version: '1', tag: 'v1', url: '${gh.origin}/asset.bin', sizeBytes: 0)),
        throwsA(isA<HelperFirmwareException>()),
      );
    });
  });
}

/// Sends every request to the fake server, whatever host was asked for.
class _RedirectingClient implements HttpClient {
  _RedirectingClient(this.origin) : _inner = HttpClient();
  final String origin;
  final HttpClient _inner;

  Uri _rewrite(Uri url) => Uri.parse('$origin${url.path}');

  @override
  Future<HttpClientRequest> getUrl(Uri url) => _inner.getUrl(_rewrite(url));

  @override
  Future<HttpClientRequest> postUrl(Uri url) => _inner.postUrl(_rewrite(url));

  @override
  void close({bool force = false}) => _inner.close(force: force);

  @override
  set connectionTimeout(Duration? value) => _inner.connectionTimeout = value;

  @override
  noSuchMethod(Invocation invocation) =>
      throw UnsupportedError('${invocation.memberName} not needed in tests');
}
