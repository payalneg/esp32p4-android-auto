/// Address and POI lookup over the flat TSV index built by
/// scripts/mapgen/builder.py:
///
///     norm \t display \t kind \t lat \t lon
///
/// `norm` is the pre-folded key the generator wrote; [normalizeQuery] has to
/// fold what the user types the same way or nothing matches. Python does it
/// with NFKD plus a small table of letters NFKD leaves alone; Dart has no
/// Unicode normaliser in the core libraries, so the fold is spelled out here
/// and test/nav/search_index_test.dart pins the cases that matter.
library;

import 'dart:convert';
import 'dart:io';
import 'dart:isolate';

import 'geo.dart';

const int kMaxSearchResults = 20;

/// Letters that NFKD does not decompose — they are distinct characters, not a
/// base plus a combining mark. Mirrors `_SPECIAL` in search.py.
const Map<String, String> _special = <String, String>{
  'ł': 'l',
  'ø': 'o',
  'đ': 'd',
  'ß': 'ss',
  'æ': 'ae',
};

/// Base letter for the accented characters that appear in Latin map data.
/// Stands in for NFKD + "drop combining marks".
const Map<String, String> _foldPairs = <String, String>{
  'àáâãäåāăą': 'a',
  'çćĉċč': 'c',
  'ďđ': 'd',
  'èéêëēĕėęě': 'e',
  'ĝğġģ': 'g',
  'ĥ': 'h',
  'ìíîïĩīĭįı': 'i',
  'ĵ': 'j',
  'ķ': 'k',
  'ĺļľŀ': 'l',
  'ñńņňŉ': 'n',
  'òóôõöøōŏő': 'o',
  'ŕŗř': 'r',
  'śŝşš': 's',
  'ţťŧ': 't',
  'ùúûüũūŭůűų': 'u',
  'ŵ': 'w',
  'ýÿŷ': 'y',
  'źżž': 'z',
};

final Map<String, String> _foldTable = () {
  final table = <String, String>{..._special};
  _foldPairs.forEach((chars, base) {
    for (final ch in chars.split('')) {
      table[ch] = base;
    }
  });
  return table;
}();

/// Lower-cases and strips diacritics: 'Floriańska' → 'florianska'.
String normalizeQuery(String s) {
  final lower = s.toLowerCase();
  final buffer = StringBuffer();
  for (final ch in lower.split('')) {
    buffer.write(_foldTable[ch] ?? ch);
  }
  return buffer.toString();
}

/// "50.0619, 19.9368" typed or pasted into the search box, as a position.
///
/// Decimal degrees, latitude first — what every map app puts on the
/// clipboard. Accepts a comma, semicolon or space between the two, and
/// decimal commas when there is a space between the pair ("50,0619 19,9368").
LatLon? parseCoordinates(String text) {
  var s = text.trim().replaceAll(';', ' ');
  final commas = s.split(',').length - 1;
  if (commas == 1) {
    s = s.replaceAll(',', ' '); // separator
  } else if (commas == 2 && s.contains(RegExp(r'\s'))) {
    s = s.replaceAll(',', '.'); // decimal commas, space-separated pair
  } else if (commas > 0) {
    return null;
  }
  final parts = s.split(RegExp(r'\s+')).where((p) => p.isNotEmpty).toList();
  if (parts.length != 2) return null;
  final lat = double.tryParse(parts[0]);
  final lon = double.tryParse(parts[1]);
  if (lat == null || lon == null) return null;
  if (lat.abs() > 90 || lon.abs() > 180) return null;
  return LatLon(lat, lon);
}

/// Five decimals: a metre, and short enough to read back.
String formatCoordinates(LatLon p) =>
    '${p.lat.toStringAsFixed(5)}, ${p.lon.toStringAsFixed(5)}';

class SearchHit {
  const SearchHit(this.display, this.kind, this.position);

  final String display;

  /// Free-form category from OSM tags ('адрес', 'cafe', 'park'…).
  final String kind;
  final LatLon position;
}

class SearchIndex {
  SearchIndex._(this._norm, this._display, this._kind, this._lat, this._lon);

  final List<String> _norm;
  final List<String> _display;
  final List<String> _kind;
  final List<double> _lat;
  final List<double> _lon;

  int get length => _norm.length;

  static SearchIndex parse(String tsv) {
    final norm = <String>[];
    final display = <String>[];
    final kind = <String>[];
    final lat = <double>[];
    final lon = <double>[];
    for (final line in const LineSplitter().convert(tsv)) {
      if (line.isEmpty) continue;
      final f = line.split('\t');
      if (f.length < 5) continue; // tolerate a truncated last line
      final la = double.tryParse(f[3]);
      final lo = double.tryParse(f[4]);
      if (la == null || lo == null) continue;
      norm.add(f[0]);
      display.add(f[1]);
      kind.add(f[2]);
      lat.add(la);
      lon.add(lo);
    }
    return SearchIndex._(norm, display, kind, lat, lon);
  }

  static Future<SearchIndex> load(String path) =>
      Isolate.run(() => parse(File(path).readAsStringSync()));

  /// Exactly what was typed, then what starts with it, then what contains
  /// every word of it, then what contains it as written.
  ///
  /// The word bucket is what makes the index usable by someone typing on a
  /// panel. Street names here are long and punctuated — "Stefana
  /// Grota-Roweckiego" — and a rider types the part they remember, in the
  /// order they remember it, without the hyphen: "grota roweckiego" matched
  /// nothing at all when a match had to be one unbroken run of characters.
  /// Each word has to start a word in the entry, so "grota" finds
  /// Grota-Roweckiego and "rota" does not; the house number is just another
  /// word, so "grota roweckiego 12" works without spelling the street out in
  /// full first.
  ///
  /// Entries are sorted by their folded key, so an exact match is always met
  /// before the longer keys that merely start with it — the early exit below
  /// cannot skip past one.
  List<SearchHit> search(String query, {int limit = kMaxSearchResults}) {
    final q = normalizeQuery(query.trim());
    if (q.isEmpty) return const <SearchHit>[];
    final words = _words(q);
    final exact = <SearchHit>[];
    final prefix = <SearchHit>[];
    final wordy = <SearchHit>[];
    final contains = <SearchHit>[];
    for (var i = 0; i < _norm.length; i++) {
      final n = _norm[i];
      if (n == q) {
        exact.add(SearchHit(_display[i], _kind[i], LatLon(_lat[i], _lon[i])));
      } else if (n.startsWith(q)) {
        prefix.add(SearchHit(_display[i], _kind[i], LatLon(_lat[i], _lon[i])));
        if (prefix.length >= limit) break;
      } else if (wordy.length < limit && _hasEveryWord(n, words)) {
        wordy.add(SearchHit(_display[i], _kind[i], LatLon(_lat[i], _lon[i])));
      } else if (contains.length < limit && n.contains(q)) {
        contains.add(SearchHit(_display[i], _kind[i], LatLon(_lat[i], _lon[i])));
      }
    }
    final out = <SearchHit>[...exact, ...prefix, ...wordy, ...contains];
    return out.length <= limit ? out : out.sublist(0, limit);
  }

  /// A hyphen is a separator to someone typing, whatever it is to a street
  /// sign; so is a full stop after "ul" and the space either side of it.
  static List<String> _words(String s) => s
      .split(RegExp(r'[^a-z0-9]+'))
      .where((w) => w.isNotEmpty)
      .toList(growable: false);

  static bool _isWordChar(int c) =>
      (c >= 0x61 && c <= 0x7A) || (c >= 0x30 && c <= 0x39);

  /// Every word of the query starts a word of [n], in any order.
  static bool _hasEveryWord(String n, List<String> words) {
    if (words.isEmpty) return false;
    for (final w in words) {
      var from = 0;
      var found = false;
      while (true) {
        final at = n.indexOf(w, from);
        if (at < 0) break;
        if (at == 0 || !_isWordChar(n.codeUnitAt(at - 1))) {
          found = true;
          break;
        }
        from = at + 1;
      }
      if (!found) return false;
    }
    return true;
  }

  /// True when this hit is what the query said, letter for letter. The panel
  /// sorts by distance, and without this an exact match for somewhere a few
  /// kilometres off would sink below the near misses.
  static bool isExact(SearchHit hit, String query) =>
      normalizeQuery(hit.display) == normalizeQuery(query.trim());
}
