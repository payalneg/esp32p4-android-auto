/// Saved maps: the thing that makes crossing a border cheap.
///
/// Downloading a region used to overwrite the only graph on the phone, so
/// coming back home meant downloading it again. These tests pin the file
/// handling behind "switch" — no network, no isolates, just directories.
library;

import 'dart:convert';
import 'dart:io';
import 'dart:typed_data';

import 'package:aa_bridge/nav/geo.dart';
import 'package:aa_bridge/nav/map_data.dart';
import 'package:aa_bridge/nav/rgf2.dart';
import 'package:aa_bridge/nav/search_index.dart';
import 'package:aa_bridge/nav/way_classes.dart';
import 'package:flutter_test/flutter_test.dart';

import 'rgf2_packer.dart';

/// A two-vertex street at [lat] degrees north.
Uint8List _graph(double lat) => packGraph(
      <(double, double)>[
        (lat, 20.0),
        (lat + 0.001, 20.0),
      ],
      <TestEdge>[
        TestEdge(
          from: 0,
          to: 1,
          lenDm: 1112,
          wayClass: kWayClasses.indexOf('residential'),
          points: <(double, double)>[
            (lat, 20.0),
            (lat + 0.001, 20.0),
          ],
        ),
      ],
    );

Future<void> _install(Directory nav, String? id, String? name, double lat) async {
  await File('${nav.path}/${MapData.graphFileName}')
      .writeAsBytes(_graph(lat), flush: true);
  final source = File('${nav.path}/${MapData.sourceFileName}');
  if (id == null) {
    if (await source.exists()) await source.delete();
  } else {
    await source.writeAsString(jsonEncode(<String, String>{
      'id': id,
      'name': name ?? id,
    }));
  }
}

/// A street spanning [south]–[north] degrees; a "country" around a province.
Future<void> _installSpan(
    Directory nav, String id, String name, double south, double north) async {
  final points = <(double, double)>[(south, 20.0), (north, 20.0)];
  final bytes = packGraph(
    points,
    <TestEdge>[
      TestEdge(
        from: 0,
        to: 1,
        lenDm: ((north - south) * 111320 * 10).round(),
        wayClass: kWayClasses.indexOf('residential'),
        points: points,
      ),
    ],
  );
  await File('${nav.path}/${MapData.graphFileName}')
      .writeAsBytes(bytes, flush: true);
  await File('${nav.path}/${MapData.sourceFileName}').writeAsString(
      jsonEncode(<String, String>{'id': id, 'name': name}));
}

MapData _mapFor(Directory nav, String? id, String? name, double lat) =>
    MapData.forTest(
      RouteGraph.parse(_graph(lat)),
      regionId: id,
      regionName: name,
      navDir: nav,
    );

void main() {
  late Directory nav;

  setUp(() async {
    nav = await Directory.systemTemp.createTemp('navlib');
  });

  tearDown(() async {
    if (await nav.exists()) await nav.delete(recursive: true);
  });

  test('a map is kept and can be switched back to', () async {
    await _install(nav, 'poland/malopolskie', 'Malopolskie', 50.0);
    final map = _mapFor(nav, 'poland/malopolskie', 'Malopolskie', 50.0);

    final first = await map.saveActiveMap();
    expect(first?.key, 'poland_malopolskie');
    expect(first?.id, 'poland/malopolskie');
    expect(await map.hasArchivedRegion('poland/malopolskie'), isTrue);
    expect((await map.archivedRegions()).single.name, 'Malopolskie');
    expect((await File('${nav.path}/${MapData.activePointerName}').readAsString()).trim(),
        'poland_malopolskie');

    // Ride into the next province: its build replaces the active graph.
    await _install(nav, 'poland/mazowieckie', 'Mazowieckie', 52.0);
    await map.reloadFromDisk();
    expect(map.regionId, 'poland/mazowieckie');
    await map.saveActiveMap();
    expect((await map.archivedRegions()).length, 2);
    expect(map.activeKey, 'poland_mazowieckie');

    // Back home, without a download.
    expect(await map.activateRegion('poland_malopolskie'), isTrue);
    expect(map.regionId, 'poland/malopolskie');
    expect(map.activeKey, 'poland_malopolskie');
    expect(map.graph!.node(0).lat, closeTo(50.0, 1e-6));
    expect((await File('${nav.path}/${MapData.activePointerName}').readAsString()).trim(),
        'poland_malopolskie');
  });

  test('an imported graph is kept too, so a region build cannot lose it',
      () async {
    await _install(nav, null, null, 49.0);
    final map = _mapFor(nav, null, null, 49.0);
    await map.reloadFromDisk();
    expect(map.regionId, isNull);

    await map.saveActiveMap();
    final saved = await map.archivedRegions();
    expect(saved.single.key, MapData.importedKey);
    expect(saved.single.isImported, isTrue);

    // A region download takes over the active slot, not the archive.
    await _install(nav, 'poland/malopolskie', 'Malopolskie', 50.0);
    await map.reloadFromDisk();
    await map.saveActiveMap();
    expect((await map.archivedRegions()).length, 2);

    expect(await map.activateRegion(MapData.importedKey), isTrue);
    expect(map.regionId, isNull);
    expect(map.graph!.node(0).lat, closeTo(49.0, 1e-6));
  });

  test('the map in use cannot be deleted from the saved list', () async {
    await _install(nav, 'poland/malopolskie', 'Malopolskie', 50.0);
    final map = _mapFor(nav, 'poland/malopolskie', 'Malopolskie', 50.0);
    await map.saveActiveMap();
    await _install(nav, 'poland/mazowieckie', 'Mazowieckie', 52.0);
    await map.reloadFromDisk();
    await map.saveActiveMap();

    await map.deleteArchived('poland/mazowieckie'); // in use
    expect(await map.hasArchivedRegion('poland/mazowieckie'), isTrue);

    await map.deleteArchived('poland_malopolskie');
    expect(await map.hasArchivedRegion('poland/malopolskie'), isFalse);
    expect((await map.archivedRegions()).single.key, 'poland_mazowieckie');
  });

  test('deleting all map data takes the saved maps with it', () async {
    await _install(nav, 'poland/malopolskie', 'Malopolskie', 50.0);
    final map = _mapFor(nav, 'poland/malopolskie', 'Malopolskie', 50.0);
    await map.saveActiveMap();

    await map.deleteAll();
    expect(map.state, MapDataState.absent);
    expect(await map.archivedRegions(), isEmpty);
    expect(File('${nav.path}/${MapData.activePointerName}').existsSync(), isFalse);
  });

  test('a missing saved map is not reported as switchable', () async {
    final map = _mapFor(nav, null, null, 50.0);
    expect(await map.hasArchivedRegion('poland/malopolskie'), isFalse);
    expect(await map.activateRegion('poland_malopolskie'), isFalse);
  });

  test('a saved map still counts as coverage after switching away', () async {
    await _install(nav, 'a', 'A', 50.000);
    final map = _mapFor(nav, 'a', 'A', 50.000);
    await map.saveActiveMap();

    await _install(nav, 'b', 'B', 50.00103);
    await map.reloadFromDisk();
    expect(map.regionId, 'b');
    // The active graph no longer has this street, but the saved one does.
    expect(map.covers(const LatLon(50.0002, 20.0)), isTrue);
  });

  test('a route continues into a saved map without switching', () async {
    await _install(nav, 'a', 'A', 50.000);
    final map = _mapFor(nav, 'a', 'A', 50.000);
    await map.saveActiveMap();

    // B starts 3 m after A ended: the two extracts were cut at the border.
    await _install(nav, 'b', 'B', 50.00103);
    await map.reloadFromDisk();
    expect(map.regionId, 'b');

    final route = await map.route(
      const LatLon(50.000, 20.0),
      const LatLon(50.00203, 20.0),
      RideProfile.bicycle,
    );
    expect(route, isNotNull);
    expect(route!.lengthM, greaterThan(220));
  });

  test('a saved country is used when the active province is inside it', () async {
    // Province active, country archived: the country's envelope contains the
    // province, so the merge must drop the active map, not the country.
    await _installSpan(nav, 'province', 'Province', 50.000, 50.001);
    final map = _mapFor(nav, 'province', 'Province', 50.000);
    await map.saveActiveMap();

    await _installSpan(nav, 'country', 'Country', 50.000, 51.500);
    await map.reloadFromDisk();
    await map.saveActiveMap();

    await _installSpan(nav, 'province', 'Province', 50.000, 50.001);
    await map.reloadFromDisk();
    expect(map.regionId, 'province');

    final route = await map.route(
      const LatLon(50.000, 20.0),
      const LatLon(51.500, 20.0),
      RideProfile.bicycle,
    );
    expect(route, isNotNull);
    expect(route!.lengthM, greaterThan(100000));
  });

  test('search indexes of saved maps are merged and kept in order', () async {
    final b = File('${nav.path}/b.tsv');
    final a = File('${nav.path}/a.tsv');
    // Deliberately out of order: the merge has to sort, because search's
    // prefix scan stops at the first limit hits.
    await b.writeAsString('warszawa\tWarszawa\tcity\t52.2297\t21.0122\n');
    await a.writeAsString('krakow\tKrakow\tcity\t50.0614\t19.9366\n');

    final index = await SearchIndex.loadMerged(<String>[b.path, a.path]);
    expect(index.length, 2);
    expect(index.search('wars').single.position.lat, closeTo(52.2297, 1e-4));
    expect(index.search('krak').single.position.lat, closeTo(50.0614, 1e-4));
  });
}
