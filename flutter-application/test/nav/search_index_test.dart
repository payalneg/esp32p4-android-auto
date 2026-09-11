import 'package:aa_bridge/nav/geo.dart';
import 'package:aa_bridge/nav/search_index.dart';
import 'package:flutter_test/flutter_test.dart';

/// The generator writes `norm \t display \t kind \t lat \t lon`.
String _row(String norm, String display, String kind, double lat, double lon) =>
    '$norm\t$display\t$kind\t$lat\t$lon';

void main() {
  group('normalizeQuery', () {
    test('folds the diacritics the generator folded', () {
      // Same cases as scripts/mapgen/search.py normalize().
      expect(normalizeQuery('Floriańska'), 'florianska');
      expect(normalizeQuery('Łobzów'), 'lobzow');
      expect(normalizeQuery('Świętokrzyska'), 'swietokrzyska');
      expect(normalizeQuery('Straße'), 'strasse');
      expect(normalizeQuery('Ørsted'), 'orsted');
    });

    test('lower-cases and leaves plain text alone', () {
      expect(normalizeQuery('Nowa Huta'), 'nowa huta');
      expect(normalizeQuery('  '), '  ');
    });
  });

  group('search', () {
    final index = SearchIndex.parse(<String>[
      _row('florianska', 'Floriańska', 'адрес', 50.064, 19.941),
      _row('ulica florianska 12', 'ulica Floriańska 12', 'адрес', 50.065, 19.941),
      _row('nowa huta', 'Nowa Huta', 'suburb', 50.072, 20.038),
      _row('kawiarnia florian', 'Kawiarnia Florian', 'cafe', 50.061, 19.937),
    ].join('\n'));

    test('parses every row', () {
      expect(index.length, 4);
    });

    test('prefix matches come before substring matches', () {
      final hits = index.search('florian');
      expect(hits.first.display, 'Floriańska');
      expect(hits.map((h) => h.display),
          containsAll(<String>['ulica Floriańska 12', 'Kawiarnia Florian']));
      expect(hits.indexWhere((h) => h.display == 'Kawiarnia Florian'),
          greaterThan(0));
    });

    test('accepts an accented query for a folded index', () {
      expect(index.search('Floriańska').first.display, 'Floriańska');
    });

    test('carries kind and position through', () {
      final hit = index.search('nowa').single;
      expect(hit.kind, 'suburb');
      expect(hit.position.lat, closeTo(50.072, 1e-9));
      expect(hit.position.lon, closeTo(20.038, 1e-9));
    });

    test('respects the limit', () {
      expect(index.search('florian', limit: 2).length, 2);
    });

    test('an empty or blank query returns nothing', () {
      expect(index.search(''), isEmpty);
      expect(index.search('   '), isEmpty);
      expect(index.search('zzzz'), isEmpty);
    });

    test('skips malformed rows instead of throwing', () {
      final messy = SearchIndex.parse(<String>[
        'too\tfew\tcolumns',
        _row('ok', 'Ok', 'x', 50.0, 20.0),
        'bad\tBad\tx\tnot-a-number\t20.0',
        '',
      ].join('\n'));
      expect(messy.length, 1);
      expect(messy.search('ok').single.display, 'Ok');
    });
  });

  group('parseCoordinates', () {
    test('accepts the forms people paste', () {
      const want = LatLon(50.0619, 19.9368);
      for (final q in <String>[
        '50.0619, 19.9368',
        '50.0619,19.9368',
        '50.0619 19.9368',
        '50.0619; 19.9368',
        '  50.0619 ,  19.9368 ',
        '50,0619 19,9368', // decimal commas, as a Polish keyboard types them
      ]) {
        final p = parseCoordinates(q);
        expect(p, isNotNull, reason: q);
        expect(p!.lat, closeTo(want.lat, 1e-9), reason: q);
        expect(p.lon, closeTo(want.lon, 1e-9), reason: q);
      }
      expect(parseCoordinates('-33.8688, 151.2093')!.lat, lessThan(0));
    });

    test('rejects what is not a pair of coordinates', () {
      for (final q in <String>[
        'Floriańska',
        '50.0619',
        '50.0619, 19.9368, 7',
        '91, 19', // no such latitude
        '50, 181',
        '50,0619,19,9368', // decimal commas without a separator: ambiguous
        '',
      ]) {
        expect(parseCoordinates(q), isNull, reason: q);
      }
    });

    test('formats to a metre and reads back', () {
      const p = LatLon(50.061947, 19.936856);
      expect(formatCoordinates(p), '50.06195, 19.93686');
      expect(parseCoordinates(formatCoordinates(p)), isNotNull);
    });
  });
}
