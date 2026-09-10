/// Runs the phone's own builder over a real Overpass answer.
///
/// The synthetic tests pin the rules; this one proves they survive contact
/// with live OSM data — 12 MB of central Kraków, the same ground the desktop
/// generator covers. Skipped unless the fixture is present:
///
///   curl -A 'AaBridgeNavigator/test' --data-urlencode \
///     'data=[out:json];(way["highway"](50.045,19.915,50.075,19.965););out body;>;out skel qt;' \
///     https://overpass-api.de/api/interpreter -o /tmp/krk.json
library;

import 'dart:io';

import 'package:aa_bridge/nav/geo.dart';
import 'package:aa_bridge/nav/graph_builder.dart';
import 'package:aa_bridge/nav/overpass.dart';
import 'package:aa_bridge/nav/rgf2.dart';
import 'package:aa_bridge/nav/router.dart';
import 'package:aa_bridge/nav/way_classes.dart';
import 'package:flutter_test/flutter_test.dart';

void main() {
  final fixture = File('/tmp/krk.json');
  if (!fixture.existsSync()) {
    // ignore: avoid_print
    print('skip: /tmp/krk.json missing — see the header of this file');
    return;
  }

  late BuiltGraph built;
  late RouteGraph graph;

  setUpAll(() {
    final sw = Stopwatch()..start();
    final body = fixture.readAsStringSync();
    final parsed = OverpassClient.parse(body, body.length);
    final afterParse = sw.elapsedMilliseconds;
    built = GraphBuilder.build(
        ways: parsed.ways,
        nodes: MapNodeSource(parsed.nodes),
        places: parsed.places);
    // ignore: avoid_print
    print('parsed ${(body.length / (1 << 20)).toStringAsFixed(1)} MB in '
        '$afterParse ms, built in ${sw.elapsedMilliseconds - afterParse} ms: '
        '${built.nodeCount} junctions, ${built.edgeCount} roads, '
        '${built.placeCount} places, '
        '${(built.graphBytes.length / (1 << 20)).toStringAsFixed(1)} MB');
    graph = RouteGraph.parse(built.graphBytes);
  });

  test('produces a graph of a plausible size for the area', () {
    // ~12 km² of a dense city centre.
    expect(built.nodeCount, greaterThan(5000));
    expect(built.edgeCount, greaterThan(8000));
    expect(built.graphBytes.length, lessThan(8 << 20));
  });

  test('the bytes are a valid RGF2 file', () {
    expect(graph.nodeCount, built.nodeCount);
    expect(graph.edgeCount, built.edgeCount);
    for (var e = 0; e < graph.edgeCount; e += 500) {
      expect(graph.edgePoints(e, true).length, greaterThanOrEqualTo(2));
      expect(graph.edgeLenDm[e], greaterThan(0));
      expect(graph.edgeClass[e], lessThan(kWayClassCount));
    }
  });

  test('every class the router knows about actually shows up', () {
    final seen = <String>{};
    for (var e = 0; e < graph.edgeCount; e++) {
      seen.add(kWayClasses[graph.edgeClass[e]]);
    }
    // A city centre has all of these; their absence would mean the
    // classification silently collapsed to one bucket.
    for (final expected in <String>[
      'residential',
      'footway',
      'sidewalk',
      'crossing',
      'cycleway',
      'service',
    ]) {
      expect(seen, contains(expected));
    }
  });

  test('one-way streets survive into the graph', () {
    var oneway = 0;
    for (var e = 0; e < graph.edgeCount; e++) {
      if (graph.edgeFlags[e] & kFlagBidir == 0) oneway++;
    }
    expect(oneway, greaterThan(100), reason: 'a city centre has one-ways');
    expect(oneway, lessThan(graph.edgeCount ~/ 2));
  });

  test('it routes across the area, and the profile changes the answer',
      () async {
    final router = Router(graph);
    // Rynek Główny to Kazimierz, both inside the fixture bounds.
    const from = LatLon(50.0619, 19.9368);
    const to = LatLon(50.0500, 19.9440);

    final scooter = await router.route(from, to, RideProfile.scooter);
    expect(scooter, isNotNull);
    expect(scooter!.lengthM, greaterThan(800));
    expect(scooter.lengthM, lessThan(4000));
    expect(scooter.points.length, greaterThan(20));

    final walk = await router.route(from, to, RideProfile.sidewalksOnly);
    expect(walk, isNotNull);
    expect(walk!.footSharePct, greaterThan(scooter.footSharePct));
    // ignore: avoid_print
    print('scooter ${scooter.lengthM.round()} m '
        '(${scooter.footSharePct.toStringAsFixed(0)}% foot), '
        'pavements ${walk.lengthM.round()} m '
        '(${walk.footSharePct.toStringAsFixed(0)}%)');
  });

  test('the search index it wrote can be read back', () {
    expect(built.placeCount, greaterThan(200));
    final first = built.searchIndexTsv.split('\n').first.split('\t');
    expect(first.length, 5);
    expect(double.tryParse(first[3]), isNotNull);
  });
}
