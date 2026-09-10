/// Builds an RGF2 routing graph out of raw OSM ways.
///
/// Port of `build_graph` in scripts/mapgen/builder.py, so a graph made on the
/// phone is byte-compatible with one made on a desktop — same vertices, same
/// classes, same file format.
///
/// The shape of the problem: OSM ways run from junction to junction and
/// beyond, so a way is cut wherever another way touches it. A node used by two
/// or more ways is a junction and becomes a vertex; everything between two
/// vertices becomes one edge that keeps its intermediate points, which is what
/// makes a route follow the curve of a street instead of cutting the corner.
library;

import 'dart:typed_data';

import 'geo.dart';
import 'osm_ways.dart';
import 'rgf2.dart';
import 'search_index.dart';

/// One OSM way, reduced to what the graph needs.
class OsmWay {
  const OsmWay({
    required this.wayClass,
    required this.direction,
    required this.nodeIds,
  });

  final int wayClass;
  final WayDirection direction;
  final List<int> nodeIds;
}

/// A place worth putting in the search index.
class OsmPlace {
  const OsmPlace(this.display, this.kind, this.position);
  final String display;
  final String kind;
  final LatLon position;
}

/// What a build produced, ready to be written to disk.
class BuiltGraph {
  const BuiltGraph({
    required this.graphBytes,
    required this.searchIndexTsv,
    required this.nodeCount,
    required this.edgeCount,
    required this.placeCount,
  });

  final Uint8List graphBytes;
  final String searchIndexTsv;
  final int nodeCount;
  final int edgeCount;
  final int placeCount;
}

class GraphBuilder {
  /// [ways] in OSM order, [nodes] their coordinates by id, [places] optional.
  static BuiltGraph build({
    required List<OsmWay> ways,
    required Map<int, LatLon> nodes,
    List<OsmPlace> places = const <OsmPlace>[],
  }) {
    // A node shared by two ways is a junction. Way endpoints count twice so
    // they are always vertices — otherwise two ways meeting end to end would
    // silently merge into one edge.
    final usage = <int, int>{};
    for (final way in ways) {
      for (final id in way.nodeIds) {
        usage[id] = (usage[id] ?? 0) + 1;
      }
      if (way.nodeIds.isNotEmpty) {
        usage[way.nodeIds.first] = (usage[way.nodeIds.first] ?? 0) + 1;
        usage[way.nodeIds.last] = (usage[way.nodeIds.last] ?? 0) + 1;
      }
    }

    final vertexOf = <int, int>{};
    final vertexLat = <double>[];
    final vertexLon = <double>[];
    int vertex(int id, LatLon at) => vertexOf.putIfAbsent(id, () {
          vertexLat.add(at.lat);
          vertexLon.add(at.lon);
          return vertexLat.length - 1;
        });

    final edges = <_Edge>[];
    for (final way in ways) {
      if (way.nodeIds.length < 2) continue;
      // Skip ways whose geometry Overpass did not deliver in full.
      if (way.nodeIds.any((id) => !nodes.containsKey(id))) continue;

      var segIds = <int>[way.nodeIds.first];
      var segPts = <LatLon>[nodes[way.nodeIds.first]!];
      for (var i = 1; i < way.nodeIds.length; i++) {
        final id = way.nodeIds[i];
        segIds.add(id);
        segPts.add(nodes[id]!);
        final isJunction = (usage[id] ?? 0) >= 2;
        if (!isJunction && i != way.nodeIds.length - 1) continue;

        var pts = segPts;
        var lengthM = 0.0;
        for (var k = 1; k < pts.length; k++) {
          lengthM += haversineM(pts[k - 1], pts[k]);
        }
        if (lengthM > 0) {
          var from = vertex(segIds.first, pts.first);
          var to = vertex(segIds.last, pts.last);
          if (way.direction == WayDirection.backward) {
            final swap = from;
            from = to;
            to = swap;
            pts = pts.reversed.toList();
          }
          edges.add(_Edge(
            from: from,
            to: to,
            lenDm: (lengthM * 10).round(),
            wayClass: way.wayClass,
            bidir: way.direction == WayDirection.both,
            points: pts,
          ));
        }
        segIds = <int>[id];
        segPts = <LatLon>[nodes[id]!];
      }
    }

    final kept = _largestComponent(vertexLat.length, edges);
    return _pack(vertexLat, vertexLon, kept, places);
  }

  /// Keeps only the biggest connected component.
  ///
  /// Without this, a courtyard path clipped by the download bounds, or a
  /// private lane behind a gate, becomes an island — and a tap that lands on
  /// it produces "no route" no matter where you are going.
  static List<_Edge> _largestComponent(int nodeCount, List<_Edge> edges) {
    if (edges.isEmpty) return edges;
    final parent = Int32List(nodeCount);
    for (var i = 0; i < nodeCount; i++) {
      parent[i] = i;
    }
    int find(int x) {
      while (parent[x] != x) {
        parent[x] = parent[parent[x]];
        x = parent[x];
      }
      return x;
    }

    for (final e in edges) {
      final a = find(e.from);
      final b = find(e.to);
      if (a != b) parent[a] = b;
    }
    final sizes = <int, int>{};
    for (final e in edges) {
      final r = find(e.from);
      sizes[r] = (sizes[r] ?? 0) + 1;
    }
    var best = -1;
    var bestRoot = -1;
    sizes.forEach((root, n) {
      if (n > best) {
        best = n;
        bestRoot = root;
      }
    });
    return edges.where((e) => find(e.from) == bestRoot).toList();
  }

  /// Renumbers the surviving vertices and writes the RGF2 blob.
  static BuiltGraph _pack(List<double> lat, List<double> lon,
      List<_Edge> edges, List<OsmPlace> places) {
    final renumber = <int, int>{};
    final outLat = <double>[];
    final outLon = <double>[];
    int keep(int v) => renumber.putIfAbsent(v, () {
          outLat.add(lat[v]);
          outLon.add(lon[v]);
          return outLat.length - 1;
        });
    for (final e in edges) {
      e.from = keep(e.from);
      e.to = keep(e.to);
    }

    var size = 12 + outLat.length * 8;
    for (final e in edges) {
      size += 16 + e.points.length * 8;
    }
    final out = Uint8List(size);
    final d = ByteData.sublistView(out);
    d.setUint32(0, kRgf2Magic, Endian.little);
    d.setUint32(4, outLat.length, Endian.little);
    d.setUint32(8, edges.length, Endian.little);
    var off = 12;
    for (var i = 0; i < outLat.length; i++) {
      d.setInt32(off, (outLat[i] * 1e7).round(), Endian.little);
      d.setInt32(off + 4, (outLon[i] * 1e7).round(), Endian.little);
      off += 8;
    }
    for (final e in edges) {
      d.setUint32(off, e.from, Endian.little);
      d.setUint32(off + 4, e.to, Endian.little);
      d.setUint32(off + 8, e.lenDm, Endian.little);
      d.setUint8(off + 12, e.wayClass);
      d.setUint8(off + 13, e.bidir ? kFlagBidir : 0);
      d.setUint16(off + 14, e.points.length, Endian.little);
      off += 16;
      for (final p in e.points) {
        d.setInt32(off, (p.lat * 1e7).round(), Endian.little);
        d.setInt32(off + 4, (p.lon * 1e7).round(), Endian.little);
        off += 8;
      }
    }

    // The index is keyed on the folded form, exactly as search.py writes it,
    // so one lookup implementation serves both sources of data.
    final seen = <String>{};
    final lines = <String>[];
    for (final p in places) {
      final display = p.display.split(RegExp(r'\s+')).join(' ').trim();
      if (display.isEmpty) continue;
      final norm = normalizeQuery(display);
      if (!seen.add('$norm/$display')) continue;
      lines.add(<String>[
        norm,
        display,
        p.kind,
        '${p.position.lat}',
        '${p.position.lon}',
      ].join('\t'));
    }
    lines.sort();

    return BuiltGraph(
      graphBytes: out,
      searchIndexTsv: lines.isEmpty ? '' : '${lines.join('\n')}\n',
      nodeCount: outLat.length,
      edgeCount: edges.length,
      placeCount: lines.length,
    );
  }
}

class _Edge {
  _Edge({
    required this.from,
    required this.to,
    required this.lenDm,
    required this.wayClass,
    required this.bidir,
    required this.points,
  });

  int from;
  int to;
  final int lenDm;
  final int wayClass;
  final bool bidir;
  final List<LatLon> points;
}
