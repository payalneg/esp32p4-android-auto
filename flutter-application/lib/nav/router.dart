/// A* over the RGF2 graph — the port of scripts/mapgen/router.py.
///
/// Edge cost is computed here, not read from the graph: length ÷ the profile's
/// speed for that edge's class. That is what makes profiles switchable without
/// touching the 17 MB file, and it means a forbidden class is simply never
/// expanded. The heuristic is the great-circle distance to the goal over the
/// profile's top speed — never an overestimate, so the result is the optimal
/// route rather than a plausible one.
///
/// Length is not the whole cost. Leaving one line group for another — the
/// carriageway, the cycleway and the pavement run parallel and touch every
/// driveway — and stepping onto a crossing each cost a flat number of seconds
/// (see kGroupSwitchPenaltyS, kCrossingPenaltyS). Without them the cheapest
/// path saws between a road and its pavement for metres at a time. The
/// penalties are non-negative and the heuristic ignores them, so it still
/// never overestimates.
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

  /// Time actually spent riding — length over the profile's speed, summed per
  /// edge. The search's own penalties for changing line or crossing a road
  /// are not in here: they steer the route, they are not minutes on the road,
  /// and this number is shown to the rider as the ETA.
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
        _group = Uint8List(graph.nodeCount),
        _viaConnector = Uint8List(graph.nodeCount),
        _open = _MinHeap(1024);

  final RouteGraph graph;

  // Scratch, reused between queries: allocating 300k-element arrays per tap
  // would cost more than the search itself.
  final Float64List _dist;

  /// How each node was reached: 0 = not reached, else ±(edge index + 1),
  /// negative meaning the edge was traversed against its stored direction.
  final Int32List _cameEdge;
  final Uint8List _closed;

  /// Line group the node was reached on, carried through connectors, so the
  /// switch penalty sees the pavement or road behind a crossing rather than
  /// the crossing itself. [kGroupNone] until the node is reached.
  final Uint8List _group;

  /// 1 when the edge that reached the node was a connector, so a chain of
  /// crossings is charged once rather than per segment.
  final Uint8List _viaConnector;
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
    _group.fillRange(0, _group.length, kGroupNone);
    _viaConnector.fillRange(0, _viaConnector.length, 0);
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
      final prevGroup = _group[u];
      final prevConnector = _viaConnector[u] != 0;
      for (var slot = graph.adjStart[u]; slot < end; slot++) {
        final e = graph.adjEdgeIndex(slot);
        final cls = graph.edgeClass[e];
        final speed = speeds[cls];
        if (speed.isNaN) continue; // class forbidden by this profile
        final v = graph.adjTarget(slot);
        if (_closed[v] != 0) continue;
        final group = kWayClassGroup[cls];
        var cost = graph.edgeLenDm[e] / 10.0 / speed;
        if (group == kGroupConnector) {
          // A crossing costs once, at the kerb, not per segment of it.
          if (!prevConnector) cost += kCrossingPenaltyS;
        } else if (prevGroup != kGroupNone && group != prevGroup) {
          cost += kGroupSwitchPenaltyS;
        }
        final nd = _dist[u] + cost;
        if (nd < _dist[v]) {
          _dist[v] = nd;
          _cameEdge[v] = graph.adjForward(slot) ? e + 1 : -(e + 1);
          // A connector is transparent: the pavement behind the crossing is
          // still what we are riding, so coming off it is not a switch.
          _group[v] = group == kGroupConnector ? prevGroup : group;
          _viaConnector[v] = group == kGroupConnector ? 1 : 0;
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
    var travelS = 0.0;
    var node = g;
    while (node != s) {
      final marker = _cameEdge[node];
      if (marker == 0) return null; // defensive: broken back-pointer chain
      final forward = marker > 0;
      final e = marker.abs() - 1;
      lengthDm += graph.edgeLenDm[e];
      travelS += graph.edgeLenDm[e] / 10.0 / speeds[graph.edgeClass[e]];
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
      timeS: travelS,
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
