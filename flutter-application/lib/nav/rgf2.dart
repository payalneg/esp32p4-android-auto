/// Reader for the RGF2 routing graph (scripts/mapgen/graph_format.py).
///
///     u32 magic = 0x32464752 ("RGF2")
///     u32 node_count, u32 edge_count
///     node_count × (i32 lat*1e7, i32 lon*1e7)
///     edge_count × (u32 from, u32 to, u32 len_dm,
///                   u8 way_class, u8 flags, u16 point_count,
///                   point_count × (i32 lat*1e7, i32 lon*1e7))
///
/// Everything lands in typed lists — a 300k-node graph is ~30 MB of Dart
/// arrays, and A* touches them in tight loops where a `List<Object>` would
/// cost a pointer chase per step. The source bytes are dropped once parsed:
/// geometry is copied into one flat [geom] array instead, which is smaller
/// than the file it came from and needs no ByteData call per coordinate.
///
/// Parsing (and the four union-finds below) takes long enough to drop frames,
/// so [load] runs it in an isolate. `Isolate.run` returns via `Isolate.exit`,
/// which *moves* the typed lists to the caller instead of copying them.
library;

import 'dart:io';
import 'dart:isolate';
import 'dart:typed_data';

import 'geo.dart';
import 'way_classes.dart';

const int kRgf2Magic = 0x32464752; // "RGF2" little-endian
const int kFlagBidir = 1;

/// Snap grid, degrees. Same cell size as router.py `_GRID` (~220 m of
/// latitude); longitude cells are 1.6× wider so they stay roughly square at
/// mid latitudes.
const double kGridLat = 0.002;
const double kGridLon = 0.002 * 1.6;

/// Points further than this from any vertex cannot be routed from.
const double kSnapMaxM = 500.0;

/// Adjacency entry: the high bit marks traversal against the stored edge
/// direction, mirroring the ADJ_FWD packing in main/map_router.c.
const int _adjReverse = 0x80000000;

class RouteGraph {
  RouteGraph._({
    required this.nodeCount,
    required this.edgeCount,
    required this.nodeLatE7,
    required this.nodeLonE7,
    required this.edgeFrom,
    required this.edgeTo,
    required this.edgeLenDm,
    required this.edgeClass,
    required this.edgeFlags,
    required this.geomStart,
    required this.geom,
    required this.adjStart,
    required this.adjEdge,
    required this.cellSlot,
    required this.cellStart,
    required this.cellNodes,
    required this.componentRoot,
    required this.componentBiggest,
  });

  final int nodeCount;
  final int edgeCount;

  final Int32List nodeLatE7;
  final Int32List nodeLonE7;

  final Uint32List edgeFrom;
  final Uint32List edgeTo;
  final Uint32List edgeLenDm;
  final Uint8List edgeClass;
  final Uint8List edgeFlags;

  /// Index into [geom] (in points, not bytes) of each edge's first vertex;
  /// length edgeCount + 1, so an edge's geometry is [start, next).
  final Uint32List geomStart;

  /// Flat lat/lon ×1e7 pairs for every edge's geometry, back to back.
  final Int32List geom;

  /// CSR adjacency: neighbours of node i are adjEdge[adjStart[i]..adjStart[i+1]).
  final Uint32List adjStart;
  final Uint32List adjEdge;

  /// Snap index: cell key → slot; slot's nodes are
  /// cellNodes[cellStart[slot]..cellStart[slot+1]).
  final Map<int, int> cellSlot;
  final Uint32List cellStart;
  final Uint32List cellNodes;

  /// Per profile: the union-find root of every node, and the root of the
  /// largest component reachable under that profile. Snapping outside the
  /// largest component is how "route not found" happens no matter where you
  /// are going — a yard whose only exit is a flight of steps, say.
  final List<Int32List> componentRoot;
  final List<int> componentBiggest;

  LatLon node(int i) => LatLon(nodeLatE7[i] / 1e7, nodeLonE7[i] / 1e7);

  /// Edge index stored in an adjacency slot.
  int adjEdgeIndex(int slot) => adjEdge[slot] & ~_adjReverse;

  /// Whether that slot traverses the edge in its stored direction.
  bool adjForward(int slot) => adjEdge[slot] & _adjReverse == 0;

  /// The vertex an adjacency slot leads to.
  int adjTarget(int slot) {
    final e = adjEdgeIndex(slot);
    return adjForward(slot) ? edgeTo[e] : edgeFrom[e];
  }

  /// Geometry of edge [e], reversed when [forward] is false. Decoded on
  /// demand — only the handful of edges on a found route ever become objects.
  List<LatLon> edgePoints(int e, bool forward) {
    final start = geomStart[e];
    final end = geomStart[e + 1];
    final n = end - start;
    final out = List<LatLon>.generate(n, (k) {
      final idx = (start + (forward ? k : n - 1 - k)) * 2;
      return LatLon(geom[idx] / 1e7, geom[idx + 1] / 1e7);
    }, growable: false);
    return out;
  }

  /// Nearest vertex to [p], or null when nothing is within [kSnapMaxM].
  ///
  /// With [profile] set, only vertices in that profile's largest component are
  /// considered, so a route from the returned vertex can actually be driven.
  int? nearestNode(LatLon p, {RideProfile? profile}) {
    final roots = profile == null ? null : componentRoot[profile.index];
    final biggest = profile == null ? -1 : componentBiggest[profile.index];
    final cx = (p.lat / kGridLat).truncate();
    final cy = (p.lon / kGridLon).truncate();
    int? best;
    var bestD = kSnapMaxM;
    for (var ring = 0; ring < 3; ring++) {
      for (var dx = -ring; dx <= ring; dx++) {
        for (var dy = -ring; dy <= ring; dy++) {
          if (dx.abs() != ring && dy.abs() != ring) continue; // ring shell only
          final slot = cellSlot[_cellKey(cx + dx, cy + dy)];
          if (slot == null) continue;
          for (var k = cellStart[slot]; k < cellStart[slot + 1]; k++) {
            final i = cellNodes[k];
            if (roots != null && roots[i] != biggest) continue;
            final d = haversineM(p, node(i));
            if (d < bestD) {
              best = i;
              bestD = d;
            }
          }
        }
      }
      if (best != null) return best; // nearest in the innermost non-empty ring
    }
    return best;
  }

  static int _cellKey(int x, int y) => (x + 0x100000) * 0x200000 + (y + 0x100000);

  /// Parses a whole RGF2 file. Throws [FormatException] on a wrong signature
  /// or a truncated file — a silent misparse would route into the sea.
  static RouteGraph parse(Uint8List bytes) {
    assert(Endian.host == Endian.little, 'RGF2 is little-endian');
    final data = ByteData.sublistView(bytes);
    if (bytes.length < 12) {
      throw const FormatException('RGF2: file shorter than its header');
    }
    final magic = data.getUint32(0, Endian.little);
    if (magic != kRgf2Magic) {
      throw FormatException('not an RGF2 routing graph: magic '
          '0x${magic.toRadixString(16)}');
    }
    final nodeCount = data.getUint32(4, Endian.little);
    final edgeCount = data.getUint32(8, Endian.little);

    const headerBytes = 12;
    const edgeHeaderBytes = 16;
    final nodeBytes = nodeCount * 8;
    final fixed = headerBytes + nodeBytes + edgeCount * edgeHeaderBytes;
    if (bytes.length < fixed) {
      throw const FormatException('RGF2: truncated before the edge table');
    }
    // Everything past the fixed-size records is edge geometry, so the total
    // point count is known up front and the flat array can be sized exactly.
    final geomBytes = bytes.length - fixed;
    if (geomBytes % 8 != 0) {
      throw const FormatException('RGF2: geometry is not whole coordinates');
    }
    final totalPoints = geomBytes ~/ 8;

    final nodeLatE7 = Int32List(nodeCount);
    final nodeLonE7 = Int32List(nodeCount);
    for (var i = 0, off = headerBytes; i < nodeCount; i++, off += 8) {
      nodeLatE7[i] = data.getInt32(off, Endian.little);
      nodeLonE7[i] = data.getInt32(off + 4, Endian.little);
    }

    final edgeFrom = Uint32List(edgeCount);
    final edgeTo = Uint32List(edgeCount);
    final edgeLenDm = Uint32List(edgeCount);
    final edgeClass = Uint8List(edgeCount);
    final edgeFlags = Uint8List(edgeCount);
    final geomStart = Uint32List(edgeCount + 1);
    final geom = Int32List(totalPoints * 2);
    final degree = Uint32List(nodeCount + 1);

    var off = headerBytes + nodeBytes;
    var pointCursor = 0;
    for (var e = 0; e < edgeCount; e++) {
      if (off + edgeHeaderBytes > bytes.length) {
        throw FormatException('RGF2: truncated edge header at $e');
      }
      final from = data.getUint32(off, Endian.little);
      final to = data.getUint32(off + 4, Endian.little);
      final lenDm = data.getUint32(off + 8, Endian.little);
      final cls = data.getUint8(off + 12);
      final flags = data.getUint8(off + 13);
      final nPts = data.getUint16(off + 14, Endian.little);
      off += edgeHeaderBytes;
      if (from >= nodeCount || to >= nodeCount) {
        throw FormatException('RGF2: edge $e points outside the node table');
      }
      if (cls >= kWayClassCount) {
        throw FormatException('RGF2: edge $e has unknown way class $cls');
      }
      if (off + nPts * 8 > bytes.length) {
        throw FormatException('RGF2: truncated geometry at edge $e');
      }
      edgeFrom[e] = from;
      edgeTo[e] = to;
      edgeLenDm[e] = lenDm;
      edgeClass[e] = cls;
      edgeFlags[e] = flags;
      geomStart[e] = pointCursor;
      for (var k = 0; k < nPts; k++) {
        final src = off + k * 8;
        final dst = (pointCursor + k) * 2;
        geom[dst] = data.getInt32(src, Endian.little);
        geom[dst + 1] = data.getInt32(src + 4, Endian.little);
      }
      pointCursor += nPts;
      off += nPts * 8;

      degree[from]++;
      if (flags & kFlagBidir != 0) degree[to]++;
    }
    geomStart[edgeCount] = pointCursor;

    // CSR: prefix-sum the degrees, then refill using the same array as a
    // write cursor (adjStart is rebuilt from it afterwards).
    final adjStart = Uint32List(nodeCount + 1);
    var running = 0;
    for (var i = 0; i < nodeCount; i++) {
      adjStart[i] = running;
      running += degree[i];
    }
    adjStart[nodeCount] = running;
    final adjEdge = Uint32List(running);
    final cursor = Uint32List.fromList(adjStart.sublist(0, nodeCount));
    for (var e = 0; e < edgeCount; e++) {
      adjEdge[cursor[edgeFrom[e]]++] = e;
      if (edgeFlags[e] & kFlagBidir != 0) {
        adjEdge[cursor[edgeTo[e]]++] = e | _adjReverse;
      }
    }

    final (cellSlot, cellStart, cellNodes) =
        _buildGrid(nodeCount, nodeLatE7, nodeLonE7);

    final componentRoot = <Int32List>[];
    final componentBiggest = <int>[];
    for (final profile in RideProfile.values) {
      final (roots, biggest) = _components(
          profile, nodeCount, edgeCount, edgeFrom, edgeTo, edgeClass);
      componentRoot.add(roots);
      componentBiggest.add(biggest);
    }

    return RouteGraph._(
      nodeCount: nodeCount,
      edgeCount: edgeCount,
      nodeLatE7: nodeLatE7,
      nodeLonE7: nodeLonE7,
      edgeFrom: edgeFrom,
      edgeTo: edgeTo,
      edgeLenDm: edgeLenDm,
      edgeClass: edgeClass,
      edgeFlags: edgeFlags,
      geomStart: geomStart,
      geom: geom,
      adjStart: adjStart,
      adjEdge: adjEdge,
      cellSlot: cellSlot,
      cellStart: cellStart,
      cellNodes: cellNodes,
      componentRoot: componentRoot,
      componentBiggest: componentBiggest,
    );
  }

  /// Reads and parses off the UI isolate.
  static Future<RouteGraph> load(String path) =>
      Isolate.run(() => parse(File(path).readAsBytesSync()));

  static (Map<int, int>, Uint32List, Uint32List) _buildGrid(
      int nodeCount, Int32List latE7, Int32List lonE7) {
    final slotOf = <int, int>{};
    final keys = Int32List(nodeCount);
    var slots = 0;
    for (var i = 0; i < nodeCount; i++) {
      final key = _cellKey((latE7[i] / 1e7 / kGridLat).truncate(),
          (lonE7[i] / 1e7 / kGridLon).truncate());
      keys[i] = slotOf.putIfAbsent(key, () => slots++);
    }
    final start = Uint32List(slots + 1);
    for (var i = 0; i < nodeCount; i++) {
      start[keys[i] + 1]++;
    }
    for (var s = 0; s < slots; s++) {
      start[s + 1] += start[s];
    }
    final nodes = Uint32List(nodeCount);
    final cursor = Uint32List.fromList(start.sublist(0, slots));
    for (var i = 0; i < nodeCount; i++) {
      nodes[cursor[keys[i]]++] = i;
    }
    return (slotOf, start, nodes);
  }

  /// Union-find over the edges a profile is allowed to use. The biggest
  /// component is the one carrying the most allowed edges, matching
  /// Router._components in router.py.
  static (Int32List, int) _components(
    RideProfile profile,
    int nodeCount,
    int edgeCount,
    Uint32List edgeFrom,
    Uint32List edgeTo,
    Uint8List edgeClass,
  ) {
    final speeds = speedsMs(profile);
    final parent = Int32List(nodeCount);
    for (var i = 0; i < nodeCount; i++) {
      parent[i] = i;
    }
    int find(int x) {
      while (parent[x] != x) {
        parent[x] = parent[parent[x]]; // path halving
        x = parent[x];
      }
      return x;
    }

    for (var e = 0; e < edgeCount; e++) {
      if (speeds[edgeClass[e]].isNaN) continue;
      final a = find(edgeFrom[e]);
      final b = find(edgeTo[e]);
      if (a != b) parent[a] = b;
    }
    final roots = Int32List(nodeCount);
    for (var i = 0; i < nodeCount; i++) {
      roots[i] = find(i);
    }
    final sizes = <int, int>{};
    for (var e = 0; e < edgeCount; e++) {
      if (speeds[edgeClass[e]].isNaN) continue;
      final r = roots[edgeFrom[e]];
      sizes[r] = (sizes[r] ?? 0) + 1;
    }
    var biggest = -1;
    var best = -1;
    sizes.forEach((root, count) {
      if (count > best) {
        best = count;
        biggest = root;
      }
    });
    return (roots, biggest);
  }
}
