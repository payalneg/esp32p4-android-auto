import 'dart:convert';
import 'dart:io';
import 'dart:isolate';
import 'dart:math' as math;

import 'geo.dart';
import 'tile_math.dart';

const _bucketZoom = 13;
const _northWest = LatLon(50.13, 19.79);
const _southEast = LatLon(49.97, 20.22);
final _coverage = bboxTiles(_northWest, _southEast, _bucketZoom);

enum KrakowFeature {
  green,
  water,
  river,
  path,
  road,
  mainRoad,
  bikeLane,
  bikePath,
  building,
  bridge,
  bikeBridge;

  static KrakowFeature? fromTags(Map<String, String> tags, bool closed) {
    if (closed) {
      if (tags['natural'] == 'water' || tags['waterway'] == 'riverbank') {
        return water;
      }
      if (tags['leisure'] == 'park' ||
          tags['landuse'] == 'forest' ||
          tags['natural'] == 'wood') {
        return green;
      }
      if (tags['building'] != null && tags['building'] != 'no') return building;
    }
    if (tags['waterway'] == 'river') return river;
    final highway = tags['highway'];
    if (highway == null || tags['area'] == 'yes') return null;
    final isBridge = ![null, 'no', 'false'].contains(tags['bridge']);
    if (highway == 'cycleway') return isBridge ? bikeBridge : bikePath;
    if (const {'path', 'footway', 'pedestrian'}.contains(highway)) {
      if (tags['bicycle'] == 'designated') {
        return isBridge ? bikeBridge : bikePath;
      }
      return isBridge ? bikeBridge : path;
    }
    if (isBridge) return bridge;
    if (const {
      'cycleway',
      'cycleway:left',
      'cycleway:right',
      'cycleway:both',
    }.any(
      (key) => const {
        'lane',
        'track',
        'opposite_lane',
        'opposite_track',
      }.contains(tags[key]),
    )) {
      return bikeLane;
    }
    if (const {
      'motorway',
      'trunk',
      'primary',
      'secondary',
      'tertiary',
    }.contains(highway)) {
      return mainRoad;
    }
    return const {
          'residential',
          'living_street',
          'unclassified',
          'service',
        }.contains(highway)
        ? road
        : null;
  }

  static String? landmarkName(Map<String, String> tags) {
    if (!const {'castle', 'monument'}.contains(tags['historic']) &&
        !const {'museum', 'attraction'}.contains(tags['tourism'])) {
      return null;
    }
    final name = tags['name'];
    return switch (name) {
      'Zamek Królewski na Wawelu' || 'Zamek Wawel' => 'Wawel',
      'Muzeum Sztuki i Techniki Japońskiej Manggha' => 'Muzeum Manggha',
      _ => name,
    };
  }
}

enum KrakowLabelKind { landmark, bridge, river, street }

typedef KrakowPoint = ({int x, int y});
typedef KrakowLabel =
    ({
      KrakowLabelKind kind,
      String name,
      KrakowPoint point,
      double bearing,
      double rank,
    });
typedef KrakowShape = ({KrakowFeature kind, List<List<KrakowPoint>> rings});
typedef KrakowScene = ({List<KrakowShape> shapes, List<KrakowLabel> labels});

const _bucketPixels = 256 << (19 - _bucketZoom);

/// Spatial buckets of OSM geometry, in Web Mercator pixels at z19.
class KrakowTileBuilder {
  final _features =
      <
        ({
          KrakowFeature? kind,
          List<int> refs,
          List<List<int>> holes,
          String? label,
          String? name,
        })
      >[];

  void add(
    KrakowFeature? kind,
    List<int> refs, {
    List<List<int>> holes = const [],
    String? label,
    String? name,
  }) => _features.add((
    kind: kind,
    refs: refs,
    holes: holes,
    label: label,
    name: name,
  ));

  Future<void> write(String path, LatLon? Function(int id) lookup) async {
    final buckets = <TileId, List<List<int?>>>{};
    final labels = <TileId, List<List<Object>>>{};
    List<KrakowPoint> project(List<int> refs) {
      final points = <KrakowPoint>[];
      for (final ref in refs) {
        final p = lookup(ref);
        if (p == null) return [];
        final m = deg2tileF(p.lat, p.lon, _bucketZoom);
        points.add((
          x: (m.x * _bucketPixels).round(),
          y: (m.y * _bucketPixels).round(),
        ));
      }
      return points;
    }

    void writeLabel(
      KrakowLabelKind kind,
      String name,
      KrakowPoint point,
      double bearing,
      double rank,
    ) {
      if (name.trim().isEmpty) return;
      final tile = TileId(
        _bucketZoom,
        point.x ~/ _bucketPixels,
        point.y ~/ _bucketPixels,
      );
      (labels[tile] ??= []).add([
        kind.index,
        name,
        point.x,
        point.y,
        bearing,
        rank,
      ]);
    }

    for (final (id, feature) in _features.indexed) {
      final points = project(feature.refs);
      if (points.isEmpty) continue;
      var xMin = _coverage.xMax + 1;
      var yMin = _coverage.yMax + 1;
      var xMax = _coverage.xMin - 1;
      var yMax = _coverage.yMin - 1;
      for (final p in points) {
        xMin = math.min(xMin, p.x ~/ _bucketPixels);
        yMin = math.min(yMin, p.y ~/ _bucketPixels);
        xMax = math.max(xMax, p.x ~/ _bucketPixels);
        yMax = math.max(yMax, p.y ~/ _bucketPixels);
      }
      final mid = points.length ~/ 2;
      final label = feature.label;
      if (label != null) {
        final anchor = (
          x:
              (points.map((p) => p.x).reduce(math.min) +
                  points.map((p) => p.x).reduce(math.max)) ~/
              2,
          y:
              (points.map((p) => p.y).reduce(math.min) +
                  points.map((p) => p.y).reduce(math.max)) ~/
              2,
        );
        writeLabel(KrakowLabelKind.landmark, label, anchor, 0, 0);
      }
      final kind = feature.kind;
      if (kind == null) continue;
      final name = feature.name;
      final labelKind = switch (kind) {
        KrakowFeature.bridge ||
        KrakowFeature.bikeBridge => KrakowLabelKind.bridge,
        KrakowFeature.river => KrakowLabelKind.river,
        KrakowFeature.mainRoad => KrakowLabelKind.street,
        _ => null,
      };
      if (name != null && labelKind != null && points.length > 1) {
        final a = points[math.max(0, mid - 2)],
            b = points[math.min(points.length - 1, mid + 2)];
        final dx = (b.x - a.x).toDouble(), dy = (b.y - a.y).toDouble();
        final bearing =
            (math.atan2(dy, dx) + math.pi / 2) % math.pi - math.pi / 2;
        writeLabel(labelKind, name, points[mid], bearing, dx * dx + dy * dy);
      }
      final geometry = <int?>[
        id,
        kind.index,
        for (final p in points) ...[p.x, p.y],
      ];
      for (final refs in feature.holes) {
        final hole = project(refs);
        if (hole.isEmpty || !_inside(hole.first, points)) continue;
        geometry.addAll([
          null,
          null,
          for (final p in hole) ...[p.x, p.y],
        ]);
      }
      for (
        var x = math.max(xMin, _coverage.xMin);
        x <= math.min(xMax, _coverage.xMax);
        x++
      ) {
        for (
          var y = math.max(yMin, _coverage.yMin);
          y <= math.min(yMax, _coverage.yMax);
          y++
        ) {
          (buckets[TileId(_bucketZoom, x, y)] ??= []).add(geometry);
        }
      }
    }
    if (buckets.isEmpty) throw const FormatException('No Krakow map geometry');
    await Directory(path).create(recursive: true);
    for (final entry in buckets.entries) {
      await File(
        '$path/${entry.key.x}-${entry.key.y}.json',
      ).writeAsString(jsonEncode(entry.value));
    }
    for (final entry in labels.entries) {
      await File(
        '$path/${entry.key.x}-${entry.key.y}.labels.json',
      ).writeAsString(jsonEncode(entry.value));
    }
  }

  static bool _inside(KrakowPoint p, List<KrakowPoint> ring) {
    var inside = false;
    for (var i = 1; i < ring.length; i++) {
      final a = ring[i - 1], b = ring[i];
      if ((a.y > p.y) != (b.y > p.y) &&
          p.x < a.x + (b.x - a.x) * (p.y - a.y) / (b.y - a.y)) {
        inside = !inside;
      }
    }
    return inside;
  }
}

class KrakowTileSource {
  KrakowTileSource(this.path, this.version);

  final String path;
  final String version;

  static ({int xMin, int xMax, int yMin, int yMax}) _bounds(TileId tile) {
    final scale = math.pow(2, _bucketZoom - tile.z).toDouble();
    return (
      xMin: (tile.x * scale).floor(),
      xMax: ((tile.x + 1) * scale).ceil() - 1,
      yMin: (tile.y * scale).floor(),
      yMax: ((tile.y + 1) * scale).ceil() - 1,
    );
  }

  bool covers(TileId tile) {
    if (tile.z < 11 || tile.z > 19) return false;
    final b = _bounds(tile);
    return b.xMin <= _coverage.xMax &&
        b.xMax >= _coverage.xMin &&
        b.yMin <= _coverage.yMax &&
        b.yMax >= _coverage.yMin;
  }

  Future<KrakowScene> loadScene(TileId tile) =>
      Isolate.run(() => _loadScene(path, tile));

  static KrakowScene _loadScene(String path, TileId tile) {
    final scale = 1 << (19 - tile.z);
    final left = tile.x * 256 * scale, top = tile.y * 256 * scale;
    final right = left + 256 * scale, bottom = top + 256 * scale;
    final geometryMargin = scale * 10;
    final labelMargin = scale * 96;
    final margin = tile.z >= 15 ? labelMargin : geometryMargin;
    final shapes = <KrakowShape>[], labels = <KrakowLabel>[];
    final seen = <int>{};
    for (
      var x = (left - margin) ~/ _bucketPixels;
      x <= (right + margin) ~/ _bucketPixels;
      x++
    ) {
      for (
        var y = (top - margin) ~/ _bucketPixels;
        y <= (bottom + margin) ~/ _bucketPixels;
        y++
      ) {
        final file = File('$path/$x-$y.json');
        if (file.existsSync()) {
          for (final row
              in (jsonDecode(file.readAsStringSync()) as List).cast<List>()) {
            if (!seen.add(row[0] as int)) continue;
            final kind = KrakowFeature.values[row[1] as int];
            if (tile.z < 15 && kind == KrakowFeature.path ||
                tile.z < 16 && kind == KrakowFeature.building) {
              continue;
            }
            final rings = <List<KrakowPoint>>[[]];
            const world = 256 << 19;
            var xMin = world, xMax = 0, yMin = world, yMax = 0;
            for (var i = 2; i + 1 < row.length; i += 2) {
              final px = row[i] as int?, py = row[i + 1] as int?;
              if (px == null || py == null) {
                rings.add([]);
                continue;
              }
              rings.last.add((x: px, y: py));
              xMin = math.min(xMin, px);
              xMax = math.max(xMax, px);
              yMin = math.min(yMin, py);
              yMax = math.max(yMax, py);
            }
            if (xMax < left - geometryMargin ||
                xMin > right + geometryMargin ||
                yMax < top - geometryMargin ||
                yMin > bottom + geometryMargin) {
              continue;
            }
            if (kind == KrakowFeature.building &&
                tile.z == 16 &&
                (xMax - xMin) * (yMax - yMin) < 45 * scale * scale) {
              continue;
            }
            shapes.add((kind: kind, rings: rings));
          }
        }
        if (tile.z < 15) continue;
        final labelFile = File('$path/$x-$y.labels.json');
        if (!labelFile.existsSync()) continue;
        for (final row
            in (jsonDecode(labelFile.readAsStringSync()) as List)
                .cast<List>()) {
          final point = (x: row[2] as int, y: row[3] as int);
          if (point.x < left - labelMargin ||
              point.x > right + labelMargin ||
              point.y < top - labelMargin ||
              point.y > bottom + labelMargin) {
            continue;
          }
          labels.add((
            kind: KrakowLabelKind.values[row[0] as int],
            name: row[1] as String,
            point: point,
            bearing: (row[4] as num).toDouble(),
            rank: (row[5] as num).toDouble(),
          ));
        }
      }
    }
    return (shapes: shapes, labels: labels);
  }
}
