/// Parity against the Python reference on the real Kraków data.
///
/// The synthetic tests pin the rules; this one pins the answers. Numbers come
/// from scripts/mapgen (router.py + navigator.py) on
/// vtiles/route_graph.bin — the same file the app downloads. Skipped when the
/// data has not been generated, so a fresh checkout still runs green:
///
///     cd scripts/mapgen && python3 main.py --download-pbf && python3 main.py --build
library;

import 'dart:io';

import 'package:aa_bridge/nav/geo.dart';
import 'package:aa_bridge/nav/maneuvers.dart';
import 'package:aa_bridge/nav/rgf2.dart';
import 'package:aa_bridge/nav/router.dart';
import 'package:aa_bridge/nav/way_classes.dart';
import 'package:flutter_test/flutter_test.dart';

const rynek = LatLon(50.0619, 19.9368);
const nowaHuta = LatLon(50.0722, 20.0378);

File? _graphFile() {
  for (final p in <String>[
    '../scripts/mapgen/vtiles/route_graph.bin',
    'scripts/mapgen/vtiles/route_graph.bin',
    '../../scripts/mapgen/vtiles/route_graph.bin',
  ]) {
    final f = File(p);
    if (f.existsSync()) return f;
  }
  return null;
}

void main() {
  final file = _graphFile();
  if (file == null) {
    // ignore: avoid_print
    print('skip: no route_graph.bin — run scripts/mapgen/main.py --build');
    return;
  }

  late RouteGraph graph;
  late Router router;

  setUpAll(() {
    final sw = Stopwatch()..start();
    graph = RouteGraph.parse(file.readAsBytesSync());
    // ignore: avoid_print
    print('parsed ${file.lengthSync() >> 20} MB graph in ${sw.elapsedMilliseconds} ms');
    router = Router(graph);
  });

  test('the file is the graph the generator reported', () {
    expect(graph.nodeCount, 302642);
    expect(graph.edgeCount, 376081);
  });

  test('scooter route matches router.py', () async {
    final sw = Stopwatch()..start();
    final r = await router.route(rynek, nowaHuta, RideProfile.scooter);
    // ignore: avoid_print
    print('scooter route: ${sw.elapsedMilliseconds} ms');
    expect(r, isNotNull);
    expect(r!.lengthM, closeTo(8049.8, 8049.8 * 0.01));
    expect(r.timeS, closeTo(1563.0, 1563.0 * 0.01));
    expect(r.points.length, closeTo(699, 40));
  });

  test('bicycle takes a different, slower way — as in Python', () async {
    final r = await router.route(rynek, nowaHuta, RideProfile.bicycle);
    expect(r!.lengthM, closeTo(8209.6, 8209.6 * 0.01));
    expect(r.timeS, closeTo(1832.3, 1832.3 * 0.01));
  });

  test('maneuver count matches navigator.py', () async {
    final r = await router.route(rynek, nowaHuta, RideProfile.scooter);
    final m = buildManeuvers(r!.points);
    expect(m.length, closeTo(16, 3));
    expect(m.last.type, ManeuverType.arrive);
  });

  test('banning the carriageway pushes the route onto pavements', () async {
    final scooter = await router.route(rynek, nowaHuta, RideProfile.scooter);
    final walk = await router.route(rynek, nowaHuta, RideProfile.sidewalksOnly);
    expect(walk, isNotNull, reason: 'must still find a way');
    expect(walk!.footSharePct, greaterThan(scooter!.footSharePct + 10));
    // Length is NOT a reliable invariant: banning the carriageway can even
    // shorten a city route by opening up pedestrian shortcuts (here 8.01 km
    // against 8.05 km). Time is what pays for it.
    expect(walk.timeS, greaterThan(scooter.timeS));
  });

  test('every profile can route across the city', () async {
    for (final p in RideProfile.values) {
      final r = await router.route(rynek, nowaHuta, p);
      expect(r, isNotNull, reason: 'no route for ${p.pythonName}');
      // ignore: avoid_print
      print('${p.pythonName}: ${(r!.lengthM / 1000).toStringAsFixed(2)} km, '
          '${(r.timeS / 60).round()} min, '
          'sidewalks ${r.footSharePct.toStringAsFixed(1)}%');
    }
  });

  test('routing time is fit for the UI thread', () async {
    final sw = Stopwatch()..start();
    for (var i = 0; i < 5; i++) {
      await router.route(rynek, nowaHuta, RideProfile.scooter);
    }
    final per = sw.elapsedMilliseconds / 5;
    // ignore: avoid_print
    print('mean route time: ${per.toStringAsFixed(1)} ms');
    expect(per, lessThan(1000), reason: 'if this trips, move A* to an isolate');
  });
}
