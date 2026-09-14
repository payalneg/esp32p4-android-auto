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

    test('the place itself comes before the streets named after it', () {
      // The bug this pins: "tyniec" found every address on Tyniecka and never
      // Tyniec, because a name that is a prefix of many longer ones has no
      // way to win on prefix rank alone.
      final tsv = <String>[
        'tyniec\tTyniec\tvillage\t50.0217\t19.8617',
        'tyniecka 1\tTyniecka 1\taddress\t50.0400\t19.8900',
        'tyniecka 2\tTyniecka 2\taddress\t50.0401\t19.8901',
        'tyniecka 3\tTyniecka 3\taddress\t50.0402\t19.8902',
      ].join('\n');
      final hits = SearchIndex.parse(tsv).search('tyniec');
      expect(hits.first.display, 'Tyniec');
      expect(hits.first.kind, 'village');
      expect(hits.length, 4); // the street is still offered, just not first
    });

    test('words match in any order, across hyphens and gaps', () {
      // What a rider types on the panel: the part of the name they remember,
      // without the punctuation. None of this matched when a hit had to be
      // one unbroken run of characters.
      final index = SearchIndex.parse(<String>[
        'stefana grota-roweckiego 12\tStefana Grota-Roweckiego 12\taddress\t50.07\t19.91',
        'grunwaldzka 5\tGrunwaldzka 5\taddress\t50.05\t19.93',
      ].join('\n'));

      for (final q in <String>[
        'grota roweckiego',   // no hyphen
        'stefa grota',        // half a word, then a later one
        'roweckiego stefana', // the other way round
        'grota 12',           // street and number without the rest
      ]) {
        expect(index.search(q).map((h) => h.display), contains('Stefana Grota-Roweckiego 12'),
            reason: q);
      }
    });

    test('a word has to start a word, not land mid-one', () {
      final index = SearchIndex.parse(
          'stefana grota-roweckiego 12\tStefana Grota-Roweckiego 12\taddress\t50.07\t19.91');
      // 'rota' is inside 'grota' — a match there would make every long name a
      // hit for every short string, which is the noise this is meant to avoid.
      expect(index.search('rota roweckiego'), isEmpty);
      // ...but as a whole word it still works from the start of one.
      expect(index.search('grota').length, 1);
    });

    test('isExact only says yes to the whole name', () {
      final hits = SearchIndex.parse(
              'tyniec\tTyniec\tvillage\t50.0217\t19.8617\n'
              'tyniecka 1\tTyniecka 1\taddress\t50.04\t19.89')
          .search('tyniec');
      expect(SearchIndex.isExact(hits[0], 'tyniec'), isTrue);
      expect(SearchIndex.isExact(hits[0], ' Tyniec '), isTrue);
      expect(SearchIndex.isExact(hits[1], 'tyniec'), isFalse);
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
