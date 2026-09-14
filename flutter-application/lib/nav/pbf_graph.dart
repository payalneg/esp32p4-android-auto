/// Builds a routing graph straight out of a regional .osm.pbf — the same
/// input, and the same rules, as scripts/mapgen.
///
/// What it costs: reading the file twice, and a decoder for a format nothing
/// in Dart reads. What it buys: a whole province in one 190 MB download,
/// which is the only practical way to route beyond a neighbourhood.
library;

import 'dart:isolate';
import 'dart:typed_data';

import 'geo.dart';
import 'graph_builder.dart';
import 'osm_ways.dart';
import 'pbf.dart';

/// Progress of a build, as a fraction and a stage the UI can name.
class PbfProgress {
  const PbfProgress(this.stageKey, this.fraction, [this.count = 0]);
  final String stageKey;
  final double fraction;

  /// How many of the thing this stage is reading have gone by — roads,
  /// points, addresses. A province is minutes of work, and a bar with no
  /// number on it is indistinguishable from a hang.
  final int count;
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
    // Addresses and POIs tagged on a building outline rather than on a point.
    // This is where nearly all of them live: a Geofabrik extract of
    // Liechtenstein has 345 addresses on nodes and 12200 on ways, and reading
    // only the nodes left the index with a few per cent of the houses — a
    // street would be there and the number on it would not.
    final addrWays = <_AddrWay>[];
    await file.forEachBlock(
      (block) {
        for (final group in block.groups) {
          _readWays(block, group, ways, needed, addrWays);
        }
      },
      onProgress: (read, total) => onProgress
          ?.call(PbfProgress('mapdata.pbf.ways', 0.5 * read / total,
              ways.length)),
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
      onProgress: (read, total) => onProgress?.call(PbfProgress(
          'mapdata.pbf.nodes', 0.5 + 0.4 * read / total, places.length)),
      cancelled: cancelled,
    );

    // Placing the addresses read off outlines is its own stage: on a province
    // it is hundreds of thousands of binary searches, long enough that a
    // progress bar frozen on "reading points" looks like a hang.
    onProgress?.call(
        PbfProgress('mapdata.pbf.addresses', 0.9, addrWays.length));

    // The first corner of the outline stands in for the building. A house is
    // metres across and the index is searched by name, so the centre would
    // buy nothing for the extra pass over every node the outline names.
    for (final a in addrWays) {
      final at = _indexOf(unique, a.nodeId);
      if (at < 0 || seen[at] == 0) continue;
      places.add(OsmPlace(a.display, a.kind,
          LatLon(latE7[at] / 1e7, lonE7[at] / 1e7)));
    }

    onProgress?.call(
        PbfProgress('mapdata.pbf.graph', 0.95, places.length));
    return GraphBuilder.build(
      ways: ways,
      nodes: SortedNodeSource(unique, latE7, lonE7, seen),
      places: places,
    );
  }

  /// Same work, off the calling isolate, with the stages sent back over a
  /// port — without them the screen sat on "loading" for the several minutes
  /// a province takes, with nothing to say whether it was working or wedged.
  static Future<BuiltGraph> buildInIsolate(String path,
      {void Function(PbfProgress)? onProgress}) async {
    final port = ReceivePort();
    final sub = port.listen((Object? message) {
      if (message is List && message.length == 3) {
        onProgress?.call(PbfProgress(
            message[0] as String, message[1] as double, message[2] as int));
      }
    });
    try {
      return await _spawn(path, port.sendPort);
    } finally {
      await sub.cancel();
      port.close();
    }
  }

  /// The closure handed to [Isolate.run] is built here and nowhere else.
  ///
  /// Dart gives a function one closure context, shared by every closure in it
  /// — so writing this inside [buildInIsolate] would drag that method's locals
  /// across with it: the ReceivePort, its subscription, and the caller's
  /// progress callback, which reaches MapData and from there the whole widget
  /// tree. The isolate refuses all of that with "object is unsendable", and
  /// the region build dies before it reads a byte. In here the context holds
  /// two sendable things and nothing else.
  static Future<BuiltGraph> _spawn(String path, SendPort send) =>
      Isolate.run(() => _buildAndReport(path, send));

  static Future<BuiltGraph> _buildAndReport(String path, SendPort send) =>
      buildAsync(path,
          onProgress: (p) =>
              send.send(<Object>[p.stageKey, p.fraction, p.count]));

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
      List<OsmWay> ways, _IdBuffer needed, List<_AddrWay> addrWays) {
    final r = ProtoReader(group);
    while (!r.isDone) {
      final (field, wire) = r.readTag();
      if (field != 3) {
        r.skip(wire);
        continue;
      }
      final way = r.readBytes();
      _readWay(block, way, ways, needed, addrWays);
    }
  }

  static void _readWay(PrimitiveBlock block, Uint8List bytes,
      List<OsmWay> ways, _IdBuffer needed, List<_AddrWay> addrWays) {
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
    // An outline carries its address whether or not it is routable, so this
    // runs before the class check turns everything that is not a road away.
    final entry = searchEntry(tags);
    if (entry != null) {
      addrWays.add(_AddrWay(entry.display, entry.kind, refs.first));
      needed.addAll(<int>[refs.first]);
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

/// An address or POI read off a way, waiting for the coordinates of the node
/// that will stand for it.
class _AddrWay {
  const _AddrWay(this.display, this.kind, this.nodeId);
  final String display;
  final String kind;
  final int nodeId;
}

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
