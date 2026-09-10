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
    //
    // The ids go straight into a typed buffer that grows in place. A province
    // names on the order of ten million of them, and collecting into a plain
    // List only to copy it into an Int64List would hold both at once.
    final ways = <OsmWay>[];
    final needed = _IdBuffer();
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

    // A sorted id list plus parallel coordinate arrays: a province's road
    // network is millions of nodes, and a HashMap of them would not fit.
    // Coordinates are kept as the same 1e-7 integers the graph file stores,
    // which halves what doubles would cost.
    final unique = _dedupe(needed.sorted());
    final latE7 = Int32List(unique.length);
    final lonE7 = Int32List(unique.length);
    final seen = Uint8List(unique.length);

    // Pass two: the coordinates of exactly those nodes, plus named places.
    final places = <OsmPlace>[];
    await file.forEachBlock(
      (block) {
        for (final group in block.groups) {
          _readNodes(block, group, unique, latE7, lonE7, seen, places);
        }
      },
      onProgress: (read, total) => onProgress
          ?.call(PbfProgress('mapdata.pbf.nodes', 0.5 + 0.5 * read / total)),
      cancelled: cancelled,
    );

    onProgress?.call(const PbfProgress('mapdata.pbf.graph', 0.98));
    return GraphBuilder.build(
      ways: ways,
      nodes: SortedNodeSource(unique, latE7, lonE7, seen),
      places: places,
    );
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
      List<OsmWay> ways, _IdBuffer needed) {
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
      List<OsmWay> ways, _IdBuffer needed) {
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
    Int32List latE7,
    Int32List lonE7,
    Uint8List seen,
    List<OsmPlace> places,
  ) {
    final r = ProtoReader(group);
    while (!r.isDone) {
      final (field, wire) = r.readTag();
      if (field == 2) {
        _readDenseNodes(block, r.readBytes(), wanted, latE7, lonE7, seen,
            places);
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
    Int32List latE7,
    Int32List lonE7,
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
        latE7[index] = (latDeg * 1e7).round();
        lonE7[index] = (lonDeg * 1e7).round();
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

/// A growable Int64List. Dart has no such thing, and a plain List of ten
/// million ids costs the same memory twice the moment it is copied into a
/// typed list to be sorted.
class _IdBuffer {
  Int64List _data = Int64List(1 << 16);
  int _length = 0;

  void addAll(List<int> values) {
    if (_length + values.length > _data.length) {
      var capacity = _data.length;
      while (capacity < _length + values.length) {
        capacity *= 2;
      }
      _data = Int64List(capacity)..setRange(0, _length, _data);
    }
    _data.setRange(_length, _length + values.length, values);
    _length += values.length;
  }

  /// The ids, sorted in place. The buffer must not be used afterwards.
  Int64List sorted() {
    final view = Int64List.sublistView(_data, 0, _length);
    view.sort();
    return view;
  }
}
