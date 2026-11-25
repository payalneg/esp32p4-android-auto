/// Merging saved regions into one routable graph.
///
/// The hard part is the border: two Geofabrik extracts are cut at the same
/// line but share no vertex ids. A route only continues if vertices a few
/// metres apart on either side are joined, and it must not join graphs that
/// merely overlap (a country saved beside its province).
library;

import 'dart:io';
import 'dart:typed_data';

import 'package:aa_bridge/nav/geo.dart';
import 'package:aa_bridge/nav/rgf2.dart';
import 'package:aa_bridge/nav/router.dart';
import 'package:aa_bridge/nav/way_classes.dart';
import 'package:flutter_test/flutter_test.dart';

import 'rgf2_packer.dart';

const int _residential = 2;

/// A straight street from ([lat], 20) to ([lat] + 0.001, 20).
List<(double, double)> _nodes(double lat) => <(double, double)>[
      (lat, 20.0),
      (lat + 0.001, 20.0),
    ];

List<TestEdge> _edges(double lat) => <TestEdge>[
      TestEdge(
        from: 0,
        to: 1,
        lenDm: 1112,
        wayClass: _residential,
        points: _nodes(lat),
      ),
    ];

void main() {
  test('the stitch class is the synthetic link', () {
    expect(kWayClasses[kStitchWayClass], 'link');
    expect(kWayClassGroup[kStitchWayClass], kGroupConnector);
  });

  test('two streets across a border become one network', () {
    // A ends at 50.001; B starts 3 m further north. The two extracts were cut
    // between them.
    final a = packGraph(_nodes(50.000), _edges(50.000));
    final b = packGraph(_nodes(50.00103), _edges(50.00103));

    final merged = RouteGraph.parse(
        RouteGraph.mergeBytes(<Uint8List>[a, b]));
    expect(merged.nodeCount, 4);
    expect(merged.edgeCount, 3); // two streets + the stitch
    expect(merged.bounds!.south, closeTo(50.000, 1e-6));
    expect(merged.bounds!.north, closeTo(50.00203, 1e-6));
  });

  test('a route crosses the stitch', () async {
    final merged = RouteGraph.parse(RouteGraph.mergeBytes(<Uint8List>[
      packGraph(_nodes(50.000), _edges(50.000)),
      packGraph(_nodes(50.00103), _edges(50.00103)),
    ]));
    final route = await Router(merged).route(
      const LatLon(50.000, 20.0),
      const LatLon(50.00203, 20.0),
      RideProfile.bicycle,
    );
    expect(route, isNotNull);
    // 222 m of street plus the 3 m stitch.
    expect(route!.lengthM, greaterThan(220));
    expect(route.lengthM, lessThan(240));
  });

  test('graphs too far apart are not stitched, and the route gives up', () async {
    final merged = RouteGraph.parse(RouteGraph.mergeBytes(<Uint8List>[
      packGraph(_nodes(50.000), _edges(50.000)),
      packGraph(_nodes(50.010), _edges(50.010)),
    ]));
    expect(merged.nodeCount, 4);
    expect(merged.edgeCount, 2); // no stitch
    expect(
      await Router(merged).route(
        const LatLon(50.0002, 20.0),
        const LatLon(50.0102, 20.0),
        RideProfile.bicycle,
      ),
      isNull,
    );
  });

  test('a graph inside another is dropped instead of duplicating it', () {
    // The "country": a long street. The "province": a short one inside it.
    final country = packGraph(
      <(double, double)>[(50.0, 20.0), (50.1, 20.0)],
      <TestEdge>[
        TestEdge(
          from: 0,
          to: 1,
          lenDm: 11119,
          wayClass: _residential,
          points: <(double, double)>[(50.0, 20.0), (50.1, 20.0)],
        ),
      ],
    );
    final province = packGraph(_nodes(50.02), _edges(50.02));
    final merged = RouteGraph.parse(
        RouteGraph.mergeBytes(<Uint8List>[province, country]));
    expect(merged.nodeCount, 2);
    expect(merged.bounds!.north, closeTo(50.1, 1e-6));
  });

  test('readBounds sees a file without parsing it', () async {
    final file = await Directory.systemTemp.createTemp('navbounds');
    addTearDown(() => file.delete(recursive: true));
    final path = '${file.path}/g.bin';
    await File(path).writeAsBytes(packGraph(_nodes(50.0), _edges(50.0)));
    final bounds = RouteGraph.readBounds(path)!;
    expect(bounds.south, closeTo(50.0, 1e-6));
    expect(bounds.north, closeTo(50.001, 1e-6));
    expect(RouteGraph.readBounds('${file.path}/missing.bin'), isNull);
  });
}
