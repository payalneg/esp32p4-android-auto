import 'dart:typed_data';

import 'krakow_tiles.dart';
import 'pbf.dart';

/// Selects map features before the routing pass. Most buildings and paths in a
/// provincial extract are outside the screen's map coverage.
class KrakowPbfSelection {
  final tiles = KrakowTileBuilder();
  final _localNodes = <int>{};
  final _members = <int, List<int>>{};
  final _relations = <_AreaRelation>[];

  bool touchesMap(List<int> nodes) => nodes.any(_localNodes.contains);

  void collect(PrimitiveBlock block) {
    for (final group in block.groups) {
      final reader = ProtoReader(group);
      while (!reader.isDone) {
        final (field, wire) = reader.readTag();
        switch (field) {
          case 2:
            _collectNodes(block, reader.readBytes());
          case 4:
            _collectRelation(block, reader.readBytes());
          default:
            reader.skip(wire);
        }
      }
    }
  }

  void _collectNodes(PrimitiveBlock block, Uint8List bytes) {
    final packed = _packedFields(bytes, const {1, 8, 9});
    final ids = packed[1], lats = packed[8], lons = packed[9];
    if (ids == null || lats == null || lons == null) return;
    var id = 0, lat = 0, lon = 0;
    while (!ids.isDone && !lats.isDone && !lons.isDone) {
      id += ids.readSVarint();
      lat += lats.readSVarint();
      lon += lons.readSVarint();
      // Pad the map's drawing extent for ways crossing its edge.
      final y = block.lat(lat), x = block.lon(lon);
      if (y >= 49.94 && y <= 50.16 && x >= 19.72 && x <= 20.29) {
        _localNodes.add(id);
      }
    }
  }

  void _collectRelation(PrimitiveBlock block, Uint8List bytes) {
    final packed = _packedFields(bytes, const {2, 3, 8, 9, 10});
    final keys = packed[2], values = packed[3];
    if (keys == null || values == null) return;
    final tags = <String, String>{};
    while (!keys.isDone && !values.isDone) {
      final key = keys.readVarint(), value = values.readVarint();
      if (key < block.strings.length && value < block.strings.length) {
        tags[block.strings[key]] = block.strings[value];
      }
    }
    if (tags['type'] != 'multipolygon') return;
    final kind = switch (KrakowFeature.fromTags(tags, true)) {
      KrakowFeature.water => KrakowFeature.water,
      KrakowFeature.green => KrakowFeature.green,
      KrakowFeature.building => KrakowFeature.building,
      _ => null,
    };
    final label = KrakowFeature.landmarkName(tags);
    if (kind == null && label == null) return;
    final roles = packed[8], refs = packed[9], types = packed[10];
    if (roles == null || refs == null || types == null) return;
    final outer = <int>[], inner = <int>[];
    var ref = 0;
    while (!refs.isDone && !roles.isDone && !types.isDone) {
      ref += refs.readSVarint();
      final role = block.strings[roles.readVarint()];
      if (types.readVarint() != 1) continue; // way member
      if (role == 'inner') {
        inner.add(ref);
      } else if (role == 'outer' || role.isEmpty) {
        outer.add(ref);
      }
    }
    if (outer.isEmpty) return;
    _relations.add((kind: kind, label: label, outer: outer, inner: inner));
    for (final id in [...outer, ...inner]) {
      _members[id] = const [];
    }
  }

  static Map<int, ProtoReader> _packedFields(Uint8List bytes, Set<int> fields) {
    final reader = ProtoReader(bytes);
    final packed = <int, ProtoReader>{};
    while (!reader.isDone) {
      final (field, wire) = reader.readTag();
      if (fields.contains(field) && wire == 2) {
        packed[field] = ProtoReader(reader.readBytes());
      } else {
        reader.skip(wire);
      }
    }
    return packed;
  }

  bool addWay(int id, Map<String, String> tags, List<int> nodes) {
    if (_members.containsKey(id)) _members[id] = nodes;
    if (!touchesMap(nodes)) return false;
    final closed = nodes.first == nodes.last;
    final kind = KrakowFeature.fromTags(tags, closed);
    final landmark = KrakowFeature.landmarkName(tags);
    if (kind == null && landmark == null) return false;
    tiles.add(kind, nodes, label: landmark, name: tags['name']);
    return true;
  }

  /// Retain relation member coordinates only for areas touching the city.
  Iterable<List<int>> finishWays() sync* {
    for (final relation in _relations) {
      final outer = relation.outer.map((id) => _members[id]!).toList();
      if (!outer.any(touchesMap)) continue;
      final inner = relation.inner.map((id) => _members[id]!);
      final holes = _joinRings(inner);
      for (final ring in _joinRings(outer)) {
        if (!touchesMap(ring)) continue;
        tiles.add(relation.kind, ring, holes: holes, label: relation.label);
        yield ring;
      }
      yield* holes;
    }
    _relations.clear();
    _members.clear();
    _localNodes.clear();
  }

  static List<List<int>> _joinRings(Iterable<List<int>> ways) {
    final open = ways.where((way) => way.length > 1).toList();
    final rings = <List<int>>[];
    while (open.isNotEmpty) {
      final ring = List<int>.of(open.removeLast());
      while (ring.last != ring.first) {
        final next = open.indexWhere(
          (way) => way.first == ring.last || way.last == ring.last,
        );
        if (next < 0) break;
        final way = open.removeAt(next);
        ring.addAll((way.first == ring.last ? way : way.reversed).skip(1));
      }
      if (ring.length >= 4 && ring.first == ring.last) rings.add(ring);
    }
    return rings;
  }
}

typedef _AreaRelation =
    ({KrakowFeature? kind, String? label, List<int> outer, List<int> inner});
