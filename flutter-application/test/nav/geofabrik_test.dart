/// The region catalogue is what turns "download a map" into a real choice, so
/// its parsing has to survive whatever Geofabrik's index contains.
library;

import 'package:aa_bridge/nav/geofabrik.dart';
import 'package:flutter_test/flutter_test.dart';

const String kIndex = '''
{"type": "FeatureCollection", "features": [
  {"type": "Feature", "properties": {
     "id": "poland", "name": "Poland", "parent": "europe",
     "urls": {"pbf": "https://download.geofabrik.de/europe/poland-latest.osm.pbf"}}},
  {"type": "Feature", "properties": {
     "id": "poland/malopolskie", "name": "Małopolskie", "parent": "poland",
     "urls": {"pbf": "https://download.geofabrik.de/europe/poland/malopolskie-latest.osm.pbf"}}},
  {"type": "Feature", "properties": {
     "id": "europe", "name": "Europe",
     "urls": {"pbf": "https://download.geofabrik.de/europe-latest.osm.pbf"}}},
  {"type": "Feature", "properties": {"id": "broken", "name": "No URLs"}},
  {"type": "Feature", "properties": {"name": "No id",
     "urls": {"pbf": "https://example.org/x.osm.pbf"}}},
  "not an object"
]}
''';

void main() {
  test('strips the markup Geofabrik leaves in its names', () {
    // Their index is generated from web pages, so several names carry a
    // literal <br /> between the local and English forms.
    expect(cleanName('Województwo małopolskie<br />(Lesser Poland)'),
        'Województwo małopolskie (Lesser Poland)');
    expect(cleanName('Bavaria'), 'Bavaria');
    expect(cleanName('A<BR/>B'), 'A B');
    expect(cleanName('  spaced   out  '), 'spaced out');
  });

  test('names come out of the catalogue already cleaned', () {
    final index = GeofabrikIndex.parse('''
      {"features": [{"properties": {"id": "x", "name": "Foo<br />(Bar)",
        "urls": {"pbf": "https://example.org/x.osm.pbf"}}}]}
    ''');
    expect(index.regions.single.name, 'Foo (Bar)');
  });

  test('reads the regions and skips the malformed entries', () {
    final index = GeofabrikIndex.parse(kIndex);
    expect(index.regions.map((r) => r.id),
        <String>['poland', 'poland/malopolskie', 'europe']);
    final mal = index.byId['poland/malopolskie']!;
    expect(mal.name, 'Małopolskie');
    expect(mal.parent, 'poland');
    expect(mal.pbfUrl.path, endsWith('malopolskie-latest.osm.pbf'));
  });

  test('builds a readable path from the parent chain', () {
    final index = GeofabrikIndex.parse(kIndex);
    expect(index.byId['poland/malopolskie']!.pathLabel(index.byId),
        'Europe / Poland / Małopolskie');
    expect(index.byId['europe']!.pathLabel(index.byId), 'Europe');
  });

  group('search', () {
    final index = GeofabrikIndex.parse(kIndex);

    test('matches on name and on id', () {
      expect(index.search('mało').single.id, 'poland/malopolskie');
      expect(index.search('malopolskie').single.id, 'poland/malopolskie');
    });

    test('the more specific region comes first', () {
      // Searching "poland" finds both the country and the province; a rider
      // asking for a map wants the smaller download offered first.
      final hits = index.search('poland');
      expect(hits.first.id, 'poland/malopolskie');
      expect(hits.map((r) => r.id), contains('poland'));
    });

    test('an empty query asks for nothing', () {
      expect(index.search(''), isEmpty);
      expect(index.search('   '), isEmpty);
      expect(index.search('atlantis'), isEmpty);
    });

    test('respects the limit', () {
      expect(index.search('o', limit: 2).length, 2);
    });
  });

  test('anything that is not a catalogue is reported as such', () {
    for (final body in <String>[
      'nonsense',
      '[]',
      '{"features": 5}',
      '{"features": []}',
      '{"features": [{"properties": {"id": "x"}}]}',
    ]) {
      expect(
          () => GeofabrikIndex.parse(body),
          throwsA(isA<GeofabrikException>().having(
              (e) => e.messageKey, 'key', 'mapdata.err.catalogFormat')),
          reason: body);
    }
  });
}
