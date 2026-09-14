/// A* over the RGF2 graph — the port of scripts/mapgen/router.py.
///
/// Edge cost is computed here, not read from the graph: length ÷ the profile's
/// speed for that edge's class. That is what makes profiles switchable without
/// touching the 17 MB file, and it means a forbidden class is simply never
/// expanded. The heuristic is the great-circle distance to the goal over the
/// profile's top speed — never an overestimate, so the result is the optimal
/// route rather than a plausible one.
library;

import 'dart:math' as math;
import 'dart:typed_data';

import 'geo.dart';
import 'rgf2.dart';
import 'way_classes.dart';

/// One found route.
class RouteResult {
  RouteResult({
    required this.points,
    required this.lengthM,
    required this.timeS,
    required this.legs,
    required this.profile,
  });

  /// Full geometry, start to finish, following the curve of every street.
  final List<LatLon> points;
  final double lengthM;
  final double timeS;

  /// (way class, length in decimetres) per edge traversed, in travel order.
  final List<({int wayClass, int lenDm})> legs;
  final RideProfile profile;

  /// Share of the route ridden on pavements and footpaths, 0..100.
  double get footSharePct {
    var total = 0;
    var foot = 0;
    for (final leg in legs) {
      total += leg.lenDm;
      if (kFootClassMask & (1 << leg.wayClass) != 0) foot += leg.lenDm;
    }
    return total == 0 ? 0 : 100.0 * foot / total;
  }
}

class Router {
  Router(this.graph)
      : _dist = Float64List(graph.nodeCount),
        _cameEdge = Int32List(graph.nodeCount),
        _closed = Uint8List(graph.nodeCount),
        _open = _MinHeap(1024);

  final RouteGraph graph;

  // Scratch, reused between queries: allocating 300k-element arrays per tap
  // would cost more than the search itself.
  final Float64List _dist;

  /// How each node was reached: 0 = not reached, else ±(edge index + 1),
  /// negative meaning the edge was traversed against its stored direction.
  final Int32List _cameEdge;
  final Uint8List _closed;
  final _MinHeap _open;

  /// Routes between two taps. Returns null when either end cannot be snapped,
  /// the ends coincide, or the goal is unreachable.
  ///
  /// Async so the search can move into a worker isolate later without
  /// touching any caller.
  Future<RouteResult?> route(
    LatLon start,
    LatLon goal,
    RideProfile profile,
  ) async {
    final s = graph.nearestNode(start, profile: profile);
    final g = graph.nearestNode(goal, profile: profile);
    if (s == null || g == null || s == g) return null;

    final speeds = speedsMs(profile);
    final vmax = maxSpeedMs(profile);
    final goalPos = graph.node(g);

    _dist.fillRange(0, _dist.length, double.infinity);
    _cameEdge.fillRange(0, _cameEdge.length, 0);
    _closed.fillRange(0, _closed.length, 0);
    _open.clear();

    _dist[s] = 0;
    _open.push(haversineM(graph.node(s), goalPos) / vmax, s);

    var found = false;
    while (!_open.isEmpty) {
      final u = _open.pop();
      if (u == g) {
        found = true;
        break;
      }
      if (_closed[u] != 0) continue; // stale heap entry
      _closed[u] = 1;
      final end = graph.adjStart[u + 1];
      for (var slot = graph.adjStart[u]; slot < end; slot++) {
        final e = graph.adjEdgeIndex(slot);
        final speed = speeds[graph.edgeClass[e]];
        if (speed.isNaN) continue; // class forbidden by this profile
        final v = graph.adjTarget(slot);
        if (_closed[v] != 0) continue;
        final nd = _dist[u] + graph.edgeLenDm[e] / 10.0 / speed;
        if (nd < _dist[v]) {
          _dist[v] = nd;
          _cameEdge[v] = graph.adjForward(slot) ? e + 1 : -(e + 1);
          _open.push(nd + haversineM(graph.node(v), goalPos) / vmax, v);
        }
      }
    }
    if (!found) return null;

    // Walk back from the goal, gluing each edge's geometry on the front. The
    // last point of one edge is the first of the next, so drop the seam.
    var points = <LatLon>[];
    final legs = <({int wayClass, int lenDm})>[];
    var lengthDm = 0;
    var node = g;
    while (node != s) {
      final marker = _cameEdge[node];
      if (marker == 0) return null; // defensive: broken back-pointer chain
      final forward = marker > 0;
      final e = marker.abs() - 1;
      lengthDm += graph.edgeLenDm[e];
      legs.add((wayClass: graph.edgeClass[e], lenDm: graph.edgeLenDm[e]));
      final seg = graph.edgePoints(e, forward);
      points = points.isEmpty
          ? seg
          : <LatLon>[...seg.sublist(0, seg.length - 1), ...points];
      node = forward ? graph.edgeFrom[e] : graph.edgeTo[e];
    }
    return RouteResult(
      points: points,
      lengthM: lengthDm / 10.0,
      timeS: _dist[g],
      legs: legs.reversed.toList(growable: false),
      profile: profile,
    );
  }
}

/// Binary min-heap over (priority, node) with lazy deletion — a node can be
/// pushed twice and the stale copy is skipped via the closed set.
class _MinHeap {
  _MinHeap(int capacity)
      : _key = Float64List(capacity),
        _node = Int32List(capacity);

  Float64List _key;
  Int32List _node;
  int _length = 0;

  bool get isEmpty => _length == 0;

  void clear() => _length = 0;

  void push(double key, int node) {
    if (_length == _key.length) _grow();
    var i = _length++;
    _key[i] = key;
    _node[i] = node;
    while (i > 0) {
      final parent = (i - 1) >> 1;
      if (_key[parent] <= _key[i]) break;
      _swap(parent, i);
      i = parent;
    }
  }

  int pop() {
    final top = _node[0];
    _length--;
    if (_length > 0) {
      _key[0] = _key[_length];
      _node[0] = _node[_length];
      var i = 0;
      while (true) {
        final l = 2 * i + 1;
        final r = l + 1;
        var small = i;
        if (l < _length && _key[l] < _key[small]) small = l;
        if (r < _length && _key[r] < _key[small]) small = r;
        if (small == i) break;
        _swap(small, i);
        i = small;
      }
    }
    return top;
  }

  void _swap(int a, int b) {
    final k = _key[a];
    _key[a] = _key[b];
    _key[b] = k;
    final n = _node[a];
    _node[a] = _node[b];
    _node[b] = n;
  }

  void _grow() {
    final cap = math.max(1024, _key.length * 2);
    _key = Float64List(cap)..setRange(0, _length, _key);
    _node = Int32List(cap)..setRange(0, _length, _node);
  }
}
