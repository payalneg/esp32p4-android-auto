/// Following a short link through its redirects, consent detour included.
library;

import 'dart:io';

import 'package:aa_bridge/nav/link_resolver.dart';
import 'package:flutter_test/flutter_test.dart';

void main() {
  late HttpServer server;

  setUp(() async {
    server = await HttpServer.bind(InternetAddress.loopbackIPv4, 0);
    server.listen((HttpRequest req) async {
      final res = req.response;
      switch (req.uri.path) {
        case '/short':
          res.statusCode = HttpStatus.found;
          res.headers.set(HttpHeaders.locationHeader, '/hop2');
        case '/hop2':
          res.statusCode = HttpStatus.movedPermanently;
          res.headers.set(HttpHeaders.locationHeader,
              'https://consent.google.com/m?continue=https%3A%2F%2Fwww.google.com%2Fmaps%2F%4050.06%2C19.93%2C17z&gl=PL');
        case '/loop':
          res.statusCode = HttpStatus.found;
          res.headers.set(HttpHeaders.locationHeader, '/loop');
        default:
          res.statusCode = HttpStatus.ok;
          res.write('hello');
      }
      await res.close();
    });
  });

  tearDown(() => server.close(force: true));

  Uri local(String path) => Uri.parse('http://127.0.0.1:${server.port}$path');

  test('walks redirects and unwraps the consent page without fetching it',
      () async {
    final full = await expandShortLink(local('/short'));
    expect(full, Uri.parse('https://www.google.com/maps/@50.06,19.93,17z'));
  });

  test('a final page is returned as is', () async {
    expect(await expandShortLink(local('/final')), local('/final'));
  });

  test('a redirect loop ends after a few hops, not never', () async {
    final full = await expandShortLink(local('/loop'))
        .timeout(const Duration(seconds: 20));
    expect(full, local('/loop'));
  });

  test('a dead host is null, not an exception', () async {
    expect(await expandShortLink(Uri.parse('http://127.0.0.1:1/x')), isNull);
  });

  test('unwrapConsent leaves other hosts alone', () {
    expect(unwrapConsent(Uri.parse('https://www.google.com/maps')), isNull);
  });
}
