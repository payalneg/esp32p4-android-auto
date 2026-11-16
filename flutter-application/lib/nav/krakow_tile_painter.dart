import 'dart:isolate';
import 'dart:math' as math;
import 'dart:typed_data';
import 'dart:ui' as ui;

import 'package:image/image.dart' as img;

import 'krakow_tiles.dart';
import 'tile_math.dart';

const _background = ui.Color(0xfff2f3ee);
const _water = ui.Color(0xff82cbdf);
const _order = [
  KrakowFeature.green,
  KrakowFeature.water,
  KrakowFeature.river,
  KrakowFeature.building,
  KrakowFeature.path,
  KrakowFeature.road,
  KrakowFeature.mainRoad,
  KrakowFeature.bridge,
  KrakowFeature.bikeLane,
  KrakowFeature.bikePath,
  KrakowFeature.bikeBridge,
];

Future<Uint8List> renderKrakowTile(KrakowTileSource source, TileId tile) async {
  final scene = await source.loadScene(tile);
  final scale = (1 << (19 - tile.z)).toDouble();
  final left = tile.x * 256, top = tile.y * 256;
  ui.Offset project(KrakowPoint p) =>
      ui.Offset(p.x / scale - left, p.y / scale - top);

  final recorder = ui.PictureRecorder();
  final canvas = ui.Canvas(recorder);
  canvas.drawColor(_background, ui.BlendMode.src);
  final layers = <KrakowFeature, ui.Path>{};
  for (final shape in scene.shapes) {
    // Keep polygons independent so overlapping OSM outlines don't cancel.
    final color = switch (shape.kind) {
      KrakowFeature.green => const ui.Color(0xffb8e4cd),
      KrakowFeature.water => _water,
      KrakowFeature.building => const ui.Color(0xffebe7dd),
      _ => null,
    };
    if (color != null) continue;
    final path = layers.putIfAbsent(shape.kind, ui.Path.new);
    for (final ring in shape.rings) {
      path.addPolygon(ring.map(project).toList(), false);
    }
  }
  final lineScale = tile.z < 16 ? .53 : math.min(1.6, 1 + (tile.z - 16) * .2);
  for (final kind in _order) {
    final area = switch (kind) {
      KrakowFeature.green => const ui.Color(0xffb8e4cd),
      KrakowFeature.water => _water,
      KrakowFeature.building => const ui.Color(0xffebe7dd),
      _ => null,
    };
    if (area != null) {
      final paint = ui.Paint()..color = area;
      for (final shape in scene.shapes.where((shape) => shape.kind == kind)) {
        final path = ui.Path()..fillType = ui.PathFillType.evenOdd;
        for (final ring in shape.rings) {
          path.addPolygon(ring.map(project).toList(), true);
        }
        canvas.drawPath(path, paint);
      }
      continue;
    }
    final path = layers[kind];
    if (path == null) continue;
    final (color, casing, width) = switch (kind) {
      KrakowFeature.river => (_water, null, 4.0),
      KrakowFeature.path => (const ui.Color(0xffbccbd0), null, 1.0),
      KrakowFeature.road => (
        const ui.Color(0xfff7f9fa),
        const ui.Color(0xffd4dde1),
        2.0,
      ),
      KrakowFeature.mainRoad => (
        const ui.Color(0xffe4ecef),
        const ui.Color(0xffc5d0d6),
        7.0,
      ),
      KrakowFeature.bridge => (
        const ui.Color(0xffe2eaed),
        const ui.Color(0xffa6b7c1),
        9.0,
      ),
      KrakowFeature.bikeLane => (const ui.Color(0xffb0d1c3), null, 1.3),
      KrakowFeature.bikePath => (const ui.Color(0xff84b9a8), null, 1.4),
      KrakowFeature.bikeBridge => (
        const ui.Color(0xffb6cbd0),
        const ui.Color(0xff8ca9b5),
        2.8,
      ),
      _ => throw StateError('Expected a map line'),
    };
    final paint =
        ui.Paint()
          ..style = ui.PaintingStyle.stroke
          ..strokeJoin = ui.StrokeJoin.round
          ..strokeCap = ui.StrokeCap.round;
    if (casing != null) {
      canvas.drawPath(
        path,
        paint
          ..color = casing
          ..strokeWidth = width * lineScale + 2,
      );
    }
    canvas.drawPath(
      path,
      paint
        ..color = color
        ..strokeWidth = width * lineScale,
    );
  }
  _drawLabels(canvas, scene.labels, project, tile.z);

  final picture = recorder.endRecording();
  try {
    final image = await picture.toImage(256, 256);
    try {
      final rgba =
          (await image.toByteData(format: ui.ImageByteFormat.rawRgba))!;
      return await _encodeTile(
        rgba.buffer.asUint8List(rgba.offsetInBytes, rgba.lengthInBytes),
      );
    } finally {
      image.dispose();
    }
  } finally {
    picture.dispose();
  }
}

// At 256 px an indexed PNG fits the ESP32's receive buffer even on a dense
// tile. Quantization and compression run off the UI isolate.
Future<Uint8List> _encodeTile(Uint8List bytes) => Isolate.run(() {
  final colors = Uint16List(256 * 256);
  final counts = <int, int>{};
  for (var i = 0; i < colors.length; i++) {
    final offset = i * 4;
    final key =
        ((bytes[offset] & 0xf8) << 8) |
        ((bytes[offset + 1] & 0xfc) << 3) |
        (bytes[offset + 2] >> 3);
    colors[i] = key;
    counts.update(key, (count) => count + 1, ifAbsent: () => 1);
  }
  final ranked =
      counts.keys.toList()..sort((a, b) => counts[b]!.compareTo(counts[a]!));
  final selected = ranked.take(256).toList();
  final palette = img.PaletteUint8(selected.length, 3);
  final indices = <int, int>{};
  final paletteColors = selected.map(_rgb565).toList();
  for (final (index, key) in selected.indexed) {
    final (r, g, b) = paletteColors[index];
    palette.setRgb(index, r, g, b);
    indices[key] = index;
  }
  for (final key in ranked.skip(selected.length)) {
    final (r, g, b) = _rgb565(key);
    var closest = 0, best = 1 << 30;
    for (final (index, (pr, pg, pb)) in paletteColors.indexed) {
      final dr = r - pr, dg = g - pg, db = b - pb;
      final score = dr * dr + 2 * dg * dg + db * db;
      if (score < best) {
        closest = index;
        best = score;
      }
    }
    indices[key] = closest;
  }
  final tile = img.Image(
    width: 256,
    height: 256,
    numChannels: 1,
    palette: palette,
  );
  for (var i = 0; i < colors.length; i++) {
    tile.setPixelIndex(i & 255, i >> 8, indices[colors[i]]!);
  }
  return img.encodePng(tile, level: 4, filter: img.PngFilter.none);
});

(int, int, int) _rgb565(int key) {
  final r = key >> 11, g = (key >> 5) & 63, b = key & 31;
  return ((r << 3) | (r >> 2), (g << 2) | (g >> 4), (b << 3) | (b >> 2));
}

void _drawLabels(
  ui.Canvas canvas,
  List<KrakowLabel> labels,
  ui.Offset Function(KrakowPoint) project,
  int zoom,
) {
  final occupied = <ui.Rect>[];
  final landmarks = <ui.Offset>[];
  const prominent = {'Wawel', 'Muzeum Manggha'};
  final names = <String, List<ui.Offset>>{};
  labels.sort((a, b) {
    final kind = a.kind.index.compareTo(b.kind.index);
    if (kind != 0) return kind;
    final featured =
        (prominent.contains(b.name) ? 1 : 0) -
        (prominent.contains(a.name) ? 1 : 0);
    if (featured != 0) return featured;
    final rank = b.rank.compareTo(a.rank);
    if (rank != 0) return rank;
    final x = a.point.x.compareTo(b.point.x);
    return x == 0 ? a.point.y.compareTo(b.point.y) : x;
  });
  for (final label in labels) {
    final p = project(label.point);
    final landmark = label.kind == KrakowLabelKind.landmark;
    if ((names[label.name] ?? const []).any(
          (previous) => (previous - p).distance < 180,
        ) ||
        landmark &&
            zoom <= 16 &&
            landmarks.any((previous) => (previous - p).distance < 145)) {
      continue;
    }
    final color = switch (label.kind) {
      KrakowLabelKind.landmark => const ui.Color(0xff785da5),
      KrakowLabelKind.bridge => const ui.Color(0xff43515d),
      KrakowLabelKind.river => const ui.Color(0xff307b91),
      KrakowLabelKind.street => const ui.Color(0xff49545e),
    };
    final paragraph = _paragraph(label.name, landmark, color);
    final width = paragraph.longestLine, height = paragraph.height;
    final a = label.bearing;
    final rect = ui.Rect.fromCenter(
      center: p,
      width:
          width * math.cos(a).abs() +
          height * math.sin(a).abs() +
          (landmark ? 30 : 8),
      height: width * math.sin(a).abs() + height * math.cos(a).abs() + 6,
    );
    if (occupied.any(rect.overlaps)) {
      paragraph.dispose();
      continue;
    }
    occupied.add(rect);
    (names[label.name] ??= []).add(p);
    if (landmark) landmarks.add(p);
    if (!rect.overlaps(const ui.Rect.fromLTWH(0, 0, 256, 256))) {
      paragraph.dispose();
      continue;
    }
    final halo = _paragraph(label.name, landmark, color, halo: true);
    canvas.save();
    canvas.translate(p.dx, p.dy);
    canvas.rotate(a);
    final offset = ui.Offset(-width / 2, -height / 2);
    canvas.drawParagraph(halo, offset);
    canvas.drawParagraph(paragraph, offset);
    if (landmark) {
      final icon = ui.Offset(-width / 2 - 9, 0);
      canvas.drawCircle(
        icon,
        6,
        ui.Paint()..color = const ui.Color(0xffffffff),
      );
      canvas.drawCircle(icon, 4.5, ui.Paint()..color = color);
      canvas.drawCircle(
        icon,
        1,
        ui.Paint()..color = const ui.Color(0xffffffff),
      );
    }
    canvas.restore();
    paragraph.dispose();
    halo.dispose();
  }
}

ui.Paragraph _paragraph(
  String text,
  bool landmark,
  ui.Color color, {
  bool halo = false,
}) {
  final builder = ui.ParagraphBuilder(
    ui.ParagraphStyle(
      textDirection: ui.TextDirection.ltr,
      fontFamily: 'Roboto',
      fontSize: landmark ? 13 : 11,
      fontWeight: landmark ? ui.FontWeight.w600 : ui.FontWeight.w400,
      maxLines: 1,
      ellipsis: '…',
    ),
  );
  builder.pushStyle(
    halo
        ? ui.TextStyle(
          foreground:
              ui.Paint()
                ..color = const ui.Color(0xfff9fbfa)
                ..style = ui.PaintingStyle.stroke
                ..strokeJoin = ui.StrokeJoin.round
                ..strokeWidth = 2.5,
        )
        : ui.TextStyle(color: color),
  );
  builder.addText(text);
  return builder.build()..layout(const ui.ParagraphConstraints(width: 164));
}
