/// Follows a shortened map link to the URL that actually names the place.
///
/// `maps.app.goo.gl/…` is a redirect and nothing more; so are Yandex's
/// `/maps/-/…` and Waze's `/h/…`. This walks the redirects by hand — the
/// automatic kind would happily land on a consent page and hand us that.
library;

import 'dart:async';
import 'dart:io';

const int kMaxRedirectHops = 8;
const Duration kResolveTimeout = Duration(seconds: 10);

/// The final URL, or null when the network or the chain gave out.
Future<Uri?> expandShortLink(
  Uri short, {
  HttpClient Function()? clientFactory,
}) async {
  final client = (clientFactory ?? HttpClient.new)();
  client.userAgent = 'Mozilla/5.0 (Linux; Android 14) AaBridgeNavigator';
  var uri = short;
  try {
    for (var hop = 0; hop < kMaxRedirectHops; hop++) {
      final unwrapped = unwrapConsent(uri);
      if (unwrapped != null) return unwrapped;
      final req = await client.getUrl(uri).timeout(kResolveTimeout);
      req.followRedirects = false;
      final resp = await req.close().timeout(kResolveTimeout);
      final location = resp.headers.value(HttpHeaders.locationHeader);
      if (resp.statusCode >= 300 && resp.statusCode < 400 && location != null) {
        uri = uri.resolve(location);
        continue;
      }
      return uri;
    }
    return uri;
  } on Object {
    return null; // offline, DNS, timeout — the caller says "could not read"
  } finally {
    client.close(force: true);
  }
}

/// In the EU a Google link may detour through consent.google.com, which
/// keeps the real destination in `continue`. Returns it, or null.
Uri? unwrapConsent(Uri uri) {
  if (!uri.host.startsWith('consent.google.')) return null;
  final next = uri.queryParameters['continue'];
  return next == null ? null : Uri.tryParse(next);
}
