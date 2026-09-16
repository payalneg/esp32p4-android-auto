/// Parity against the Python reference on the real Kraków data.
///
/// The synthetic tests pin the rules; this one pins the answers. Numbers come
/// from scripts/mapgen (router.py + navigator.py) on
/// vtiles/route_graph.bin — the same file the app downloads. Skipped when the
/// data has not been generated, so a fresh checkout still runs green.
///
/// The numbers moved once, when the search started paying to change line
/// group and to step onto a crossing: the scooter route went from 54 crossings
/// to 6 and got 40 m shorter but 8 % slower, because it stopped hopping onto
/// the pavement for the sake of a few metres. Re-take them from router.py if
/// a penalty changes:
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
const kazimierz = LatLon(50.0490, 19.9450);
const bronowice = LatLon(50.0800, 19.8800);
const podgorze = LatLon(50.0400, 19.9600);
const salwator = LatLon(50.0530, 19.9080);

/// Runs of one line group along a route, connectors folded into the run they
/// interrupt — a crossing is a way of staying on the pavement, not a change
/// of line. Returns the length in decimetres of each run, in travel order.
List<int> _groupRuns(List<({int wayClass, int lenDm})> legs) {
  final runs = <List<int>>[];
  for (final leg in legs) {
    final g = kWayClassGroup[leg.wayClass];
    if (g == kGroupConnector) {
      if (runs.isNotEmpty) runs.last[1] += leg.lenDm;
      continue;
    }
    if (runs.isNotEmpty && runs.last[0] == g) {
      runs.last[1] += leg.lenDm;
    } else {
      runs.add(<int>[g, leg.lenDm]);
    }
  }
  return runs.map((r) => r[1]).toList();
}

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
    expect(r!.lengthM, closeTo(8010.6, 8010.6 * 0.01));
    expect(r.timeS, closeTo(1684.4, 1684.4 * 0.01));
    expect(r.points.length, closeTo(516, 40));
  });

  test('bicycle takes a different, slower way — as in Python', () async {
    final r = await router.route(rynek, nowaHuta, RideProfile.bicycle);
    expect(r!.lengthM, closeTo(8070.4, 8070.4 * 0.01));
    expect(r.timeS, closeTo(1955.1, 1955.1 * 0.01));
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
    // Length is NOT a reliable invariant: banning the carriageway opens up
    // pedestrian shortcuts, and the two come out within a hundred metres of
    // each other (here 8.11 km against 8.01 km). Time is what pays for it.
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

  test('the route does not saw between a road and its pavement', () async {
    // The complaint this guards: "it tells me to cross over and then come
    // straight back". A carriageway, its cycleway and its pavement are
    // parallel lines joined every 30-50 m by a driveway, so without a price
    // on changing line the cheapest path hops between them for metres at a
    // time. Measured over these six rides (68 km): before the penalties,
    // 1.58 changes of line per km and 0.33 excursions shorter than 80 m;
    // after, 0.58 and 0.01.
    const rides = <(String, LatLon, LatLon)>[
      ('Rynek -> Nowa Huta', rynek, nowaHuta),
      ('Kazimierz -> Bronowice', kazimierz, bronowice),
      ('Podgorze -> Rynek', podgorze, rynek),
      ('Nowa Huta -> Bronowice', nowaHuta, bronowice),
      ('Rynek -> Salwator', rynek, salwator),
    ];
    var totalKm = 0.0;
    var switches = 0;
    var shortHops = 0;
    for (final profile in <RideProfile>[
      RideProfile.scooter,
      RideProfile.bicycle,
    ]) {
      for (final (name, from, to) in rides) {
        final r = await router.route(from, to, profile);
        expect(r, isNotNull, reason: '$name / ${profile.pythonName}');
        final runs = _groupRuns(r!.legs);
        totalKm += r.lengthM / 1000.0;
        switches += runs.length - 1;
        // A run that is neither the first nor the last and is under 80 m is
        // the round trip itself: off the pavement and back within a block.
        for (var i = 1; i < runs.length - 1; i++) {
          if (runs[i] < 800) shortHops++;
        }
      }
    }
    // ignore: avoid_print
    print('line changes: ${(switches / totalKm).toStringAsFixed(2)}/km, '
        'short hops: ${(shortHops / totalKm).toStringAsFixed(2)}/km '
        'over ${totalKm.toStringAsFixed(1)} km');
    expect(switches / totalKm, lessThan(1.0),
        reason: 'the route keeps changing line — has a penalty been lost?');
    expect(shortHops / totalKm, lessThan(0.1),
        reason: 'short excursions onto a parallel line are back');
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
