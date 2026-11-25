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
import 'dart:math' as math;
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

/// A component smaller than this is a fragment — a courtyard, a pier, a
/// service loop — not a network worth snapping a route onto. The biggest
/// component always qualifies; in a merged graph each region's main network
/// does too.
const int kMinComponentNodes = 500;

/// Two vertices this close, from different regional graphs, are the two sides
/// of one border cut. They are joined with a synthetic link so a route can
/// cross between saved maps.
const double kStitchRadiusM = 12.0;

/// `link` in [kWayClasses]: synthetic, traversable by every profile, and
/// charged as a connector rather than as a line of its own.
const int kStitchWayClass = 13;


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
    required this.componentUsable,
    required this.bounds,
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

  /// Per profile: 1 for a vertex in a component worth snapping to. The
  /// biggest component and every substantial one qualify, so a merged graph
  /// can route in any of its regions without treating a footpath fragment as
  /// a road.
  final List<Uint8List> componentUsable;

  /// The envelope of every vertex, or null for an empty graph. The file format
  /// has no header field for it, so it is computed while parsing — one pass
  /// over an array that has just been filled anyway.
  final LatLonBounds? bounds;

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

  /// Nearest vertex to [p], or null when nothing is within [maxRadiusM].
  ///
  /// With [profile] set, only vertices in that profile's largest component are
  /// considered, so a route from the returned vertex can actually be driven.
  ///
  /// [maxRadiusM] exists for "is there any road near here" questions (map
  /// coverage), where 500 m of snap radius is too strict and the answer is
  /// allowed to be approximate. It widens the number of grid rings searched
  /// rather than changing the test itself.
  int? nearestNode(LatLon p,
      {RideProfile? profile, double maxRadiusM = kSnapMaxM}) {
    final usable = profile == null ? null : componentUsable[profile.index];
    final cx = (p.lat / kGridLat).truncate();
    final cy = (p.lon / kGridLon).truncate();
    int? best;
    var bestD = maxRadiusM;
    // Grid cells are ~220 m tall, so this is the number of rings that can
    // hold a point this far away; the haversine check below is what decides.
    final rings = (maxRadiusM / 200.0).ceil().clamp(3, 60);
    for (var ring = 0; ring < rings; ring++) {
      for (var dx = -ring; dx <= ring; dx++) {
        for (var dy = -ring; dy <= ring; dy++) {
          if (dx.abs() != ring && dy.abs() != ring) continue; // ring shell only
          final slot = cellSlot[_cellKey(cx + dx, cy + dy)];
          if (slot == null) continue;
          for (var k = cellStart[slot]; k < cellStart[slot + 1]; k++) {
            final i = cellNodes[k];
            if (usable != null && usable[i] == 0) continue;
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
    final bounds = _boundsOf(nodeLatE7, nodeLonE7);

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
    final componentUsable = <Uint8List>[];
    for (final profile in RideProfile.values) {
      final (roots, biggest, usable) = _components(
          profile, nodeCount, edgeCount, edgeFrom, edgeTo, edgeClass);
      componentRoot.add(roots);
      componentBiggest.add(biggest);
      componentUsable.add(usable);
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
      componentUsable: componentUsable,
      bounds: bounds,
    );
  }

  static LatLonBounds? _boundsOf(Int32List latE7, Int32List lonE7) {
    if (latE7.isEmpty) return null;
    var minLat = latE7[0], maxLat = latE7[0];
    var minLon = lonE7[0], maxLon = lonE7[0];
    for (var i = 1; i < latE7.length; i++) {
      final la = latE7[i];
      final lo = lonE7[i];
      if (la < minLat) minLat = la;
      if (la > maxLat) maxLat = la;
      if (lo < minLon) minLon = lo;
      if (lo > maxLon) maxLon = lo;
    }
    return LatLonBounds(
      south: minLat / 1e7,
      west: minLon / 1e7,
      north: maxLat / 1e7,
      east: maxLon / 1e7,
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
  /// Router._components in router.py. Substantial components are marked too:
  /// a merged graph has one network per region, and they are not connected.
  static (Int32List, int, Uint8List) _components(
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
    final usable = Uint8List(nodeCount);
    for (var i = 0; i < nodeCount; i++) {
      final count = sizes[roots[i]] ?? 0;
      usable[i] =
          roots[i] == biggest || count >= kMinComponentNodes ? 1 : 0;
    }
    return (roots, biggest, usable);
  }

  /// Reads and merges several RGF2 files off the UI isolate.
  ///
  /// The reading happens inside the isolate: these files are tens or hundreds
  /// of megabytes, and shipping their bytes through a port to merge them here
  /// would copy everything twice.
  static Future<RouteGraph> mergeFiles(List<String> paths) =>
      Isolate.run(() {
        final sources = <Uint8List>[];
        for (final path in paths) {
          try {
            sources.add(File(path).readAsBytesSync());
          } on FileSystemException {
            // A saved map that vanished must not take the others with it.
          }
        }
        return parse(mergeBytes(sources));
      });

  /// Merges RGF2 images into one graph.
  ///
  /// A graph whose envelope sits inside a larger one is dropped: a country
  /// saved beside one of its provinces would otherwise duplicate every road
  /// inside that province. Vertices from different graphs within
  /// [kStitchRadiusM] of each other are joined with a synthetic link — that is
  /// what turns two extracts cut at the same border into one network.
  static Uint8List mergeBytes(List<Uint8List> sources) {
    final parts = <_MergePart>[];
    for (final bytes in sources) {
      if (bytes.length < 12) continue;
      final header = ByteData.sublistView(bytes);
      if (header.getUint32(0, Endian.little) != kRgf2Magic) continue;
      final nodeCount = header.getUint32(4, Endian.little);
      final edgeCount = header.getUint32(8, Endian.little);
      final nodesEnd = 12 + nodeCount * 8;
      if (bytes.length < nodesEnd) continue;
      final bounds = _boundsOfBytes(bytes, 12, nodeCount);
      final points = _pointCountOf(bytes, nodesEnd, edgeCount);
      if (points < 0) continue;
      parts.add(_MergePart(
          bytes, nodeCount, edgeCount, points, bounds));
    }
    if (parts.isEmpty) return _emptyGraph();

    // Drop the inner graph of any containing pair; overlapping siblings are
    // kept, because they meet at a border rather than duplicating each other.
    final keep = List<bool>.filled(parts.length, true);
    for (var i = 0; i < parts.length; i++) {
      final inner = parts[i].bounds;
      if (inner == null) continue;
      for (var j = 0; j < parts.length; j++) {
        if (i == j) continue;
        final outer = parts[j].bounds;
        if (outer == null) continue;
        final outerSpan = (outer.north - outer.south) + (outer.east - outer.west);
        final innerSpan = (inner.north - inner.south) + (inner.east - inner.west);
        if (outerSpan <= innerSpan * 1.02) continue;
        if (_containsBounds(outer, inner)) {
          keep[i] = false;
          break;
        }
      }
    }
    final kept = <_MergePart>[
      for (var i = 0; i < parts.length; i++)
        if (keep[i]) parts[i],
    ];
    if (kept.isEmpty) return _emptyGraph();

    var totalNodes = 0;
    for (final part in kept) {
      totalNodes += part.nodeCount;
    }
    final latE7 = Int32List(totalNodes);
    final lonE7 = Int32List(totalNodes);
    final sourceOf = Uint8List(totalNodes);
    final cosLat = Float64List(totalNodes);
    var at = 0;
    for (var s = 0; s < kept.length; s++) {
      final part = kept[s];
      final data = ByteData.sublistView(part.bytes);
      var off = 12;
      for (var i = 0; i < part.nodeCount; i++, off += 8) {
        latE7[at] = data.getInt32(off, Endian.little);
        lonE7[at] = data.getInt32(off + 4, Endian.little);
        sourceOf[at] = s;
        cosLat[at] =
            math.cos(latE7[at] / 1e7 * math.pi / 180.0).abs();
        at++;
      }
    }

    final stitches = _stitches(latE7, lonE7, sourceOf, cosLat);

    var baseEdges = 0;
    var basePoints = 0;
    for (final part in kept) {
      baseEdges += part.edgeCount;
      basePoints += part.pointCount;
    }
    final totalEdges = baseEdges + stitches.length;
    final totalPoints = basePoints + stitches.length * 2;
    final out =
        Uint8List(12 + totalNodes * 8 + totalEdges * 16 + totalPoints * 8);
    final outData = ByteData.sublistView(out);
    outData.setUint32(0, kRgf2Magic, Endian.little);
    outData.setUint32(4, totalNodes, Endian.little);
    outData.setUint32(8, totalEdges, Endian.little);

    var off = 12;
    for (var i = 0; i < totalNodes; i++) {
      outData.setInt32(off, latE7[i], Endian.little);
      outData.setInt32(off + 4, lonE7[i], Endian.little);
      off += 8;
    }

    var nodeOffset = 0;
    for (final part in kept) {
      final src = ByteData.sublistView(part.bytes);
      var srcOff = 12 + part.nodeCount * 8;
      for (var e = 0; e < part.edgeCount; e++) {
        final from = src.getUint32(srcOff, Endian.little) + nodeOffset;
        final to = src.getUint32(srcOff + 4, Endian.little) + nodeOffset;
        outData.setUint32(off, from, Endian.little);
        outData.setUint32(off + 4, to, Endian.little);
        outData.setUint32(
            off + 8, src.getUint32(srcOff + 8, Endian.little), Endian.little);
        outData.setUint8(off + 12, src.getUint8(srcOff + 12));
        outData.setUint8(off + 13, src.getUint8(srcOff + 13));
        final nPts = src.getUint16(srcOff + 14, Endian.little);
        outData.setUint16(off + 14, nPts, Endian.little);
        off += 16;
        out.setRange(off, off + nPts * 8, part.bytes, srcOff + 16);
        off += nPts * 8;
        srcOff += 16 + nPts * 8;
      }
      nodeOffset += part.nodeCount;
    }

    for (final stitch in stitches) {
      outData.setUint32(off, stitch.a, Endian.little);
      outData.setUint32(off + 4, stitch.b, Endian.little);
      outData.setUint32(off + 8, stitch.lenDm, Endian.little);
      outData.setUint8(off + 12, kStitchWayClass);
      outData.setUint8(off + 13, kFlagBidir);
      outData.setUint16(off + 14, 2, Endian.little);
      off += 16;
      outData.setInt32(off, latE7[stitch.a], Endian.little);
      outData.setInt32(off + 4, lonE7[stitch.a], Endian.little);
      outData.setInt32(off + 8, latE7[stitch.b], Endian.little);
      outData.setInt32(off + 12, lonE7[stitch.b], Endian.little);
      off += 16;
    }
    return out;
  }

  /// The envelope of a graph file, read without parsing its edges.
  ///
  /// Coverage uses this so a saved region can answer "is there a map here"
  /// without becoming an in-memory graph.
  static LatLonBounds? readBounds(String path) {
    RandomAccessFile? file;
    try {
      file = File(path).openSync();
      final head = file.readSync(12);
      if (head.length < 12) return null;
      final data = ByteData.sublistView(head);
      if (data.getUint32(0, Endian.little) != kRgf2Magic) return null;
      final nodeCount = data.getUint32(4, Endian.little);
      if (nodeCount > 50000000) return null; // a corrupt header, not a graph
      final nodes = file.readSync(nodeCount * 8);
      if (nodes.length < nodeCount * 8) return null;
      return _boundsOfBytes(nodes, 0, nodeCount);
    } on FileSystemException {
      return null;
    } finally {
      file?.closeSync();
    }
  }

  static Uint8List _emptyGraph() {
    final out = Uint8List(12);
    ByteData.sublistView(out).setUint32(0, kRgf2Magic, Endian.little);
    return out;
  }

  static List<_Stitch> _stitches(Int32List latE7, Int32List lonE7,
      Uint8List sourceOf, Float64List cosLat) {
    const cellLat = 0.00015; // ~17 m
    const cellLon = 0.0002; // ~22 m at the equator
    const radius2 = kStitchRadiusM * kStitchRadiusM;
    final n = latE7.length;
    final cellX = Int32List(n);
    final cellY = Int32List(n);
    final grid = <int, List<int>>{};
    int keyOf(int x, int y) => (x + 0x200000) * 0x800000 + (y + 0x200000);
    for (var i = 0; i < n; i++) {
      final x = (latE7[i] / 1e7 / cellLat).truncate();
      final y = (lonE7[i] / 1e7 / cellLon).truncate();
      cellX[i] = x;
      cellY[i] = y;
      grid.putIfAbsent(keyOf(x, y), () => <int>[]).add(i);
    }

    final stitches = <_Stitch>[];
    final seen = <int>{};
    for (var i = 0; i < n; i++) {
      int? bestJ;
      var bestD2 = radius2;
      for (var dx = -1; dx <= 1; dx++) {
        for (var dy = -1; dy <= 1; dy++) {
          final list = grid[keyOf(cellX[i] + dx, cellY[i] + dy)];
          if (list == null) continue;
          for (final j in list) {
            if (sourceOf[j] == sourceOf[i]) continue;
            final dLat = (latE7[j] - latE7[i]) / 1e7;
            final dLon = (lonE7[j] - lonE7[i]) / 1e7 * cosLat[i];
            final d2 = dLat * dLat + dLon * dLon;
            if (d2 < bestD2) {
              bestD2 = d2;
              bestJ = j;
            }
          }
        }
      }
      if (bestJ == null) continue;
      final a = i < bestJ ? i : bestJ;
      final b = i < bestJ ? bestJ : i;
      // Dart ints are 64-bit; n is far below 2^31, so this pair key fits.
      if (!seen.add(a * n + b)) continue;
      stitches.add(_Stitch(
          a, b, math.max(1, (math.sqrt(bestD2) * 10).round())));
      if (stitches.length >= 100000) break;
    }
    return stitches;
  }

  static LatLonBounds? _boundsOfBytes(
      Uint8List bytes, int offset, int nodeCount) {
    if (nodeCount == 0) return null;
    final data = ByteData.sublistView(bytes);
    var minLat = 0x7fffffff, maxLat = -0x80000000;
    var minLon = 0x7fffffff, maxLon = -0x80000000;
    for (var i = 0, off = offset; i < nodeCount; i++, off += 8) {
      final la = data.getInt32(off, Endian.little);
      final lo = data.getInt32(off + 4, Endian.little);
      if (la < minLat) minLat = la;
      if (la > maxLat) maxLat = la;
      if (lo < minLon) minLon = lo;
      if (lo > maxLon) maxLon = lo;
    }
    return LatLonBounds(
      south: minLat / 1e7,
      west: minLon / 1e7,
      north: maxLat / 1e7,
      east: maxLon / 1e7,
    );
  }

  static int _pointCountOf(Uint8List bytes, int edgesAt, int edgeCount) {
    final data = ByteData.sublistView(bytes);
    var off = edgesAt;
    var points = 0;
    for (var e = 0; e < edgeCount; e++) {
      if (off + 16 > bytes.length) return -1;
      final n = data.getUint16(off + 14, Endian.little);
      points += n;
      off += 16 + n * 8;
    }
    return off == bytes.length ? points : -1;
  }

  static bool _containsBounds(LatLonBounds outer, LatLonBounds inner) =>
      inner.south >= outer.south - 1e-9 &&
      inner.north <= outer.north + 1e-9 &&
      inner.west >= outer.west - 1e-9 &&
      inner.east <= outer.east + 1e-9;
}

class _MergePart {
  const _MergePart(
      this.bytes, this.nodeCount, this.edgeCount, this.pointCount, this.bounds);

  final Uint8List bytes;
  final int nodeCount;
  final int edgeCount;
  final int pointCount;
  final LatLonBounds? bounds;
}

class _Stitch {
  const _Stitch(this.a, this.b, this.lenDm);

  final int a;
  final int b;
  final int lenDm;
}
