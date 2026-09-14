/// What another app meant when it handed us a place.
///
/// Three ways it arrives: an ACTION_VIEW with a `geo:` or
/// `google.navigation:` URI, a shared piece of text ("Share" from Google
/// Maps, Yandex, OSM, a chat) with a map link somewhere in it, or a bare pair
/// of coordinates. This turns any of them into a point — or a search string,
/// or a short link that must be followed first — and says how much the
/// sender wanted: to see the place, a route to it, or guidance right now.
///
/// Pure Dart, no network: following a short link is [expandShortLink]'s job
/// in link_resolver.dart, and the result comes back through [parseMapUrl].
library;

import 'geo.dart';
import 'search_index.dart';

enum LinkIntent {
  /// "Here is a place" — show it and offer the choices.
  show,

  /// "Directions to" — build the route, let the rider press Start.
  route,

  /// "Navigate" — `google.navigation:`; start guiding.
  navigate,
}

class NavLink {
  const NavLink({
    this.point,
    this.query,
    this.expand,
    this.intent = LinkIntent.show,
  });

  /// A place, when the link carried coordinates.
  final LatLon? point;

  /// A place to look up, when it carried text instead ("Rynek Główny").
  final String? query;

  /// A shortened link (maps.app.goo.gl, yandex /maps/-/, waze /h/) that says
  /// nothing until followed over the network.
  final Uri? expand;

  final LinkIntent intent;

  @override
  String toString() => 'NavLink($point, $query, $expand, $intent)';
}

/// Anything shared or pasted: URIs of ours, map URLs, or plain coordinates.
NavLink? parseNavText(String text) {
  final t = text.trim();
  if (t.isEmpty) return null;
  for (final m in RegExp(r'(geo:|google\.navigation:)\S+').allMatches(t)) {
    final uri = Uri.tryParse(_trimTail(m.group(0)!));
    if (uri == null) continue;
    final link = parseNavLink(uri);
    if (link != null) return link;
  }
  for (final m in RegExp(r'''https?://[^\s<>"']+''').allMatches(t)) {
    final uri = Uri.tryParse(_trimTail(m.group(0)!));
    if (uri == null) continue;
    final link = parseMapUrl(uri);
    if (link != null) return link;
  }
  final point = parseCoordinates(t) ?? _findCoordinates(t);
  return point == null ? null : NavLink(point: point);
}

/// One URI: `geo:`, `google.navigation:`, or an http(s) map link.
NavLink? parseNavLink(Uri uri) {
  switch (uri.scheme) {
    case 'geo':
      return _geo(uri);
    case 'google.navigation':
      return _navigation(uri);
    case 'http':
    case 'https':
      return parseMapUrl(uri);
    default:
      return null;
  }
}

/// `geo:lat,lon`, `geo:lat,lon?z=17`, `geo:0,0?q=lat,lon(Label)`,
/// `geo:0,0?q=Some+Place`. Per the Android spec a `q` wins over the path.
NavLink? _geo(Uri uri) {
  final params = _opaqueParams(uri);
  final q = params['q'];
  if (q != null && q.trim().isNotEmpty) {
    final fromQ = _coords(q);
    if (fromQ != null) return NavLink(point: fromQ);
    return NavLink(query: q.trim());
  }
  final path = uri.path.split(';').first; // drop `;u=10` and friends
  final point = parseCoordinates(path);
  if (point == null || (point.lat == 0 && point.lon == 0)) return null;
  return NavLink(point: point);
}

/// `google.navigation:q=lat,lon`, `google.navigation:q=Some+Place&mode=b`.
NavLink? _navigation(Uri uri) {
  final q = _opaqueParams(uri)['q'];
  if (q == null || q.trim().isEmpty) return null;
  final point = _coords(q);
  if (point != null) return NavLink(point: point, intent: LinkIntent.navigate);
  return NavLink(query: q.trim(), intent: LinkIntent.navigate);
}

/// A map site's URL. Null for a URL that is not one we read.
NavLink? parseMapUrl(Uri uri) {
  final host = uri.host.toLowerCase().replaceFirst(RegExp(r'^www\.'), '');
  final path = uri.path;
  final q = _query(uri);

  // Shorteners: nothing in the URL itself, the answer is behind a redirect.
  if (host == 'maps.app.goo.gl' ||
      (host == 'goo.gl' && path.startsWith('/maps')) ||
      (host.contains('yandex.') && path.startsWith('/maps/-/')) ||
      (host.endsWith('waze.com') && path.startsWith('/h/')) ||
      (host == 'osm.org' && path.startsWith('/go/'))) {
    return NavLink(expand: uri);
  }

  if (host.contains('google.') && (path.contains('/maps') || host.startsWith('maps.'))) {
    final directions = path.contains('/dir') ||
        q.containsKey('destination') ||
        q.containsKey('daddr');
    final point = _coords(q['destination']) ??
        _coords(q['daddr']) ??
        _coords(q['q']) ??
        _coords(q['query']) ??
        _coords(q['ll']) ??
        _coords(q['center']) ??
        _googlePin(path) ??
        _googleAt(path);
    final intent = directions ? LinkIntent.route : LinkIntent.show;
    if (point != null) return NavLink(point: point, intent: intent);
    final text = _firstText(<String?>[q['destination'], q['daddr'], q['q'], q['query']]) ??
        _googlePlaceName(path);
    return text == null ? null : NavLink(query: text, intent: intent);
  }

  if (host.endsWith('openstreetmap.org')) {
    final marker = _pair(q['mlat'], q['mlon']);
    if (marker != null) return NavLink(point: marker);
    // #map=17/50.0619/19.9368
    final m = RegExp(r'map=\d+(?:\.\d+)?/(-?\d+(?:\.\d+)?)/(-?\d+(?:\.\d+)?)')
        .firstMatch(uri.fragment);
    if (m != null) return NavLink(point: _pair(m.group(1), m.group(2)));
    return null;
  }

  if (host.contains('yandex.') && path.contains('/maps')) {
    // Yandex writes lon,lat. `pt` is the pin, `whatshere` the tapped spot,
    // `rtext` a route ("from~to"), `ll` merely the viewport.
    final rtext = q['rtext'];
    final to = rtext == null ? null : rtext.split('~').last;
    final point = _lonLat(q['pt']) ??
        _lonLat(q['whatshere[point]']) ??
        _lonLat(to) ??
        _lonLat(q['ll']);
    if (point == null) return null;
    return NavLink(
        point: point, intent: to != null ? LinkIntent.route : LinkIntent.show);
  }

  if (host == 'maps.apple.com') {
    final daddr = _coords(q['daddr']);
    if (daddr != null) return NavLink(point: daddr, intent: LinkIntent.route);
    final point = _coords(q['ll']) ?? _coords(q['q']) ?? _coords(q['sll']);
    if (point != null) return NavLink(point: point);
    final text = _firstText(<String?>[q['daddr'], q['q']]);
    return text == null
        ? null
        : NavLink(query: text, intent: q.containsKey('daddr') ? LinkIntent.route : LinkIntent.show);
  }

  if (host.endsWith('waze.com')) {
    // waze.com/ul?ll=lat,lon&navigate=yes ; live-map/directions?to=ll.lat,lon
    final to = q['to'];
    final point = _coords(q['ll']) ??
        _coords(to == null ? null : to.replaceFirst(RegExp(r'^ll\.'), ''));
    if (point == null) return null;
    final route = q['navigate'] == 'yes' || to != null || path.contains('directions');
    return NavLink(point: point, intent: route ? LinkIntent.route : LinkIntent.show);
  }

  if (host.contains('2gis.')) {
    // 2GIS writes lon,lat too: ?m=38.97,45.03/16 and /geo/<id>/38.97,45.03
    final m = q['m']?.split('/').first;
    final geo = RegExp(r'/geo/[^/]+/(-?\d+\.\d+),(-?\d+\.\d+)').firstMatch(path);
    final point = _lonLat(m) ??
        (geo == null ? null : _pair(geo.group(2), geo.group(1)));
    return point == null ? null : NavLink(point: point);
  }

  return null;
}

// --- pieces ---

/// `!3dLAT!4dLON` in a Google place URL is the pin itself; `@lat,lon,17z` is
/// only where the map was looking, so the pin is preferred.
LatLon? _googlePin(String path) {
  final m = RegExp(r'!3d(-?\d+(?:\.\d+)?)!4d(-?\d+(?:\.\d+)?)').firstMatch(path);
  return m == null ? null : _pair(m.group(1), m.group(2));
}

LatLon? _googleAt(String path) {
  final m = RegExp(r'/@(-?\d+(?:\.\d+)?),(-?\d+(?:\.\d+)?)').firstMatch(path);
  return m == null ? null : _pair(m.group(1), m.group(2));
}

/// `/maps/place/Rynek+G%C5%82%C3%B3wny/@…` → "Rynek Główny".
String? _googlePlaceName(String path) {
  final m = RegExp(r'/place/([^/@]+)').firstMatch(path);
  if (m == null) return null;
  final raw = m.group(1)!.replaceAll('+', ' ');
  try {
    final name = Uri.decodeComponent(raw).trim();
    return name.isEmpty ? null : name;
  } on ArgumentError {
    return raw.trim();
  }
}

LatLon? _coords(String? s) => s == null ? null : parseCoordinates(_stripLabel(s));

/// Yandex/2GIS order: "lon,lat".
LatLon? _lonLat(String? s) {
  final p = _coords(s);
  if (p == null) return null;
  final swapped = LatLon(p.lon, p.lat);
  return swapped.lat.abs() > 90 ? null : swapped;
}

LatLon? _pair(String? lat, String? lon) {
  if (lat == null || lon == null) return null;
  return parseCoordinates('$lat $lon');
}

String? _firstText(List<String?> candidates) {
  for (final c in candidates) {
    if (c != null && c.trim().isNotEmpty) return c.trim();
  }
  return null;
}

/// Query parameters as a map; malformed percent-encoding is tolerated.
Map<String, String> _query(Uri uri) {
  try {
    return uri.queryParameters;
  } on ArgumentError {
    return const <String, String>{};
  }
}

/// Parameters wherever the scheme put them: after a `?`, or — for the
/// opaque `google.navigation:q=…` form — as the whole path.
Map<String, String> _opaqueParams(Uri uri) {
  final out = <String, String>{};
  for (final part in <String>[uri.path, uri.query]) {
    if (!part.contains('=')) continue;
    try {
      out.addAll(Uri.splitQueryString(part));
    } on ArgumentError {
      // malformed percent-encoding: ignore that half
    }
  }
  return out;
}

/// `50.06,19.93(Rynek)` → `50.06,19.93`.
String _stripLabel(String q) {
  final i = q.indexOf('(');
  return (i < 0 ? q : q.substring(0, i)).trim();
}

/// A URL at the end of a sentence drags its full stop along.
String _trimTail(String s) => s.replaceFirst(RegExp(r'[.,;:!?)\]]+$'), '');

/// "…be at 50.0619, 19.9368 by six" — a pair of decimals with enough digits
/// to be coordinates and not a price.
LatLon? _findCoordinates(String text) {
  final m = RegExp(r'(-?\d{1,2}\.\d{3,})\s*[,;]?\s+(-?\d{1,3}\.\d{3,})|(-?\d{1,2}\.\d{3,})\s*,\s*(-?\d{1,3}\.\d{3,})')
      .firstMatch(text);
  if (m == null) return null;
  return _pair(m.group(1) ?? m.group(3), m.group(2) ?? m.group(4));
}
