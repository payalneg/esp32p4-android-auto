/// Builds a routing graph straight out of a regional .osm.pbf — the same
/// input, and the same rules, as scripts/mapgen.
///
/// The trade against Overpass: a whole province is one 190 MB download here
/// instead of tens of gigabytes of JSON, at the cost of reading the file
/// twice and of a decoder for the format. Which is why this exists at all —
/// routes over a hundred kilometres are not reachable any other way.
library;

import 'dart:isolate';
import 'dart:typed_data';

import 'geo.dart';
import 'graph_builder.dart';
import 'osm_ways.dart';
import 'pbf.dart';

/// Progress of a build, as a fraction and a stage the UI can name.
class PbfProgress {
  const PbfProgress(this.stageKey, this.fraction);
  final String stageKey;
  final double fraction;
}

class PbfGraphSource {
  /// Two passes: the ways worth keeping, then the coordinates they name.
  static Future<BuiltGraph> buildAsync(
    String path, {
    void Function(PbfProgress)? onProgress,
    bool Function()? cancelled,
  }) async {
    final file = PbfFile(path);

    // Pass one: every routable way, and the ids of the nodes it uses.
    final ways = <OsmWay>[];
    final needed = <int>[];
    await file.forEachBlock(
      (block) {
        for (final group in block.groups) {
          _readWays(block, group, ways, needed);
        }
      },
      onProgress: (read, total) => onProgress
          ?.call(PbfProgress('mapdata.pbf.ways', 0.5 * read / total)),
      cancelled: cancelled,
    );
    if (cancelled?.call() ?? false) return _cancelledResult();

    // A sorted id list plus a parallel coordinate array: a province's road
    // network is millions of nodes, and a HashMap of them would not fit.
    final ids = Int64List.fromList(needed)..sort();
    final unique = _dedupe(ids);
    final lat = Float64List(unique.length);
    final lon = Float64List(unique.length);
    final seen = Uint8List(unique.length);

    // Pass two: the coordinates of exactly those nodes, plus named places.
    final places = <OsmPlace>[];
    await file.forEachBlock(
      (block) {
        for (final group in block.groups) {
          _readNodes(block, group, unique, lat, lon, seen, places);
        }
      },
      onProgress: (read, total) => onProgress
          ?.call(PbfProgress('mapdata.pbf.nodes', 0.5 + 0.5 * read / total)),
      cancelled: cancelled,
    );

    onProgress?.call(const PbfProgress('mapdata.pbf.graph', 0.98));
    final nodes = <int, LatLon>{};
    for (var i = 0; i < unique.length; i++) {
      if (seen[i] != 0) nodes[unique[i]] = LatLon(lat[i], lon[i]);
    }
    return GraphBuilder.build(ways: ways, nodes: nodes, places: places);
  }

  /// Same work, off the calling isolate.
  static Future<BuiltGraph> buildInIsolate(String path) =>
      Isolate.run(() => buildAsync(path));

  static Int64List _dedupe(Int64List sorted) {
    if (sorted.isEmpty) return sorted;
    var write = 1;
    for (var read = 1; read < sorted.length; read++) {
      if (sorted[read] != sorted[write - 1]) sorted[write++] = sorted[read];
    }
    return Int64List.sublistView(sorted, 0, write);
  }

  /// Binary search over the sorted id list; -1 when the node is not wanted.
  static int _indexOf(Int64List sorted, int id) {
    var lo = 0;
    var hi = sorted.length - 1;
    while (lo <= hi) {
      final mid = (lo + hi) >> 1;
      final v = sorted[mid];
      if (v == id) return mid;
      if (v < id) {
        lo = mid + 1;
      } else {
        hi = mid - 1;
      }
    }
    return -1;
  }

  static void _readWays(PrimitiveBlock block, Uint8List group,
      List<OsmWay> ways, List<int> needed) {
    final r = ProtoReader(group);
    while (!r.isDone) {
      final (field, wire) = r.readTag();
      if (field != 3) {
        r.skip(wire);
        continue;
      }
      final way = r.readBytes();
      _readWay(block, way, ways, needed);
    }
  }

  static void _readWay(PrimitiveBlock block, Uint8List bytes,
      List<OsmWay> ways, List<int> needed) {
    final r = ProtoReader(bytes);
    final keys = <int>[];
    final vals = <int>[];
    final refs = <int>[];
    while (!r.isDone) {
      final (field, wire) = r.readTag();
      switch (field) {
        case 2: // packed key indices
          final sub = ProtoReader(r.readBytes());
          while (!sub.isDone) {
            keys.add(sub.readVarint());
          }
        case 3: // packed value indices
          final sub = ProtoReader(r.readBytes());
          while (!sub.isDone) {
            vals.add(sub.readVarint());
          }
        case 8: // node refs, delta encoded
          final sub = ProtoReader(r.readBytes());
          var id = 0;
          while (!sub.isDone) {
            id += sub.readSVarint();
            refs.add(id);
          }
        default:
          r.skip(wire);
      }
    }
    if (refs.length < 2) return;
    final tags = <String, String>{};
    for (var i = 0; i < keys.length && i < vals.length; i++) {
      final k = keys[i];
      final v = vals[i];
      if (k < block.strings.length && v < block.strings.length) {
        tags[block.strings[k]] = block.strings[v];
      }
    }
    final wayClass = classifyWay(tags);
    if (wayClass == null) return;
    ways.add(OsmWay(
        wayClass: wayClass, direction: wayDirection(tags), nodeIds: refs));
    needed.addAll(refs);
  }

  static void _readNodes(
    PrimitiveBlock block,
    Uint8List group,
    Int64List wanted,
    Float64List lat,
    Float64List lon,
    Uint8List seen,
    List<OsmPlace> places,
  ) {
    final r = ProtoReader(group);
    while (!r.isDone) {
      final (field, wire) = r.readTag();
      if (field == 2) {
        _readDenseNodes(block, r.readBytes(), wanted, lat, lon, seen, places);
      } else {
        r.skip(wire);
      }
    }
  }

  /// DenseNodes: ids, coordinates and tags, all delta or index encoded, in
  /// parallel packed arrays. This is where nearly every node in a PBF lives.
  static void _readDenseNodes(
    PrimitiveBlock block,
    Uint8List bytes,
    Int64List wanted,
    Float64List lat,
    Float64List lon,
    Uint8List seen,
    List<OsmPlace> places,
  ) {
    final ids = <int>[];
    final lats = <int>[];
    final lons = <int>[];
    final keysVals = <int>[];
    final r = ProtoReader(bytes);
    while (!r.isDone) {
      final (field, wire) = r.readTag();
      switch (field) {
        case 1:
          final sub = ProtoReader(r.readBytes());
          var v = 0;
          while (!sub.isDone) {
            v += sub.readSVarint();
            ids.add(v);
          }
        case 8:
          final sub = ProtoReader(r.readBytes());
          var v = 0;
          while (!sub.isDone) {
            v += sub.readSVarint();
            lats.add(v);
          }
        case 9:
          final sub = ProtoReader(r.readBytes());
          var v = 0;
          while (!sub.isDone) {
            v += sub.readSVarint();
            lons.add(v);
          }
        case 10:
          final sub = ProtoReader(r.readBytes());
          while (!sub.isDone) {
            keysVals.add(sub.readVarint());
          }
        default:
          r.skip(wire);
      }
    }

    var kv = 0;
    for (var i = 0; i < ids.length && i < lats.length && i < lons.length; i++) {
      // Tags for node i run until a zero terminator, as index pairs.
      Map<String, String>? tags;
      while (kv < keysVals.length && keysVals[kv] != 0) {
        final k = keysVals[kv++];
        if (kv >= keysVals.length) break;
        final v = keysVals[kv++];
        if (k < block.strings.length && v < block.strings.length) {
          (tags ??= <String, String>{})[block.strings[k]] = block.strings[v];
        }
      }
      if (kv < keysVals.length) kv++; // step over the terminator

      final latDeg = block.lat(lats[i]);
      final lonDeg = block.lon(lons[i]);
      final index = _indexOf(wanted, ids[i]);
      if (index >= 0) {
        lat[index] = latDeg;
        lon[index] = lonDeg;
        seen[index] = 1;
      }
      if (tags != null) {
        final entry = searchEntry(tags);
        if (entry != null) {
          places.add(OsmPlace(entry.display, entry.kind, LatLon(latDeg, lonDeg)));
        }
      }
    }
  }
}

BuiltGraph _cancelledResult() => BuiltGraph(
      graphBytes: Uint8List(0),
      searchIndexTsv: '',
      nodeCount: 0,
      edgeCount: 0,
      placeCount: 0,
    );
