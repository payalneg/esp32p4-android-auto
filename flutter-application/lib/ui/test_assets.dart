/// Synthetic PNGs for the in-app test panel.
///
/// Lets us send a "WhatsApp icon" or a "Sunset Drive cover" over BLE
/// without the test target actually having WhatsApp installed or a
/// player running. All rendering happens in-process via dart:ui; the
/// result is encoded as PNG so it slots straight into BleService.sendIcon
/// / sendAlbumArt and the head unit's lv_img decoder handles it the
/// same as a real one.
library;

import 'dart:typed_data';
import 'dart:ui' as ui;

import 'package:flutter/material.dart';
import 'package:flutter/painting.dart';

/// Solid-colour square with a single bold letter at the centre. Used as
/// the per-notification app icon stand-in.
Future<ui.Image> _drawIconBitmap({
  required int size,
  required Color background,
  required String letter,
}) async {
  final recorder = ui.PictureRecorder();
  final canvas = Canvas(recorder);
  final paint = Paint()..color = background;
  final rect = Offset.zero & Size(size.toDouble(), size.toDouble());
  // Slight rounding so the icon looks like a phone launcher icon, not
  // a flat square — gives the head unit's 18 px clip a little visual
  // give and matches what real adaptive icons end up as.
  canvas.drawRRect(
    RRect.fromRectAndRadius(rect, Radius.circular(size * 0.22)),
    paint,
  );
  final tp = TextPainter(
    text: TextSpan(
      text: letter,
      style: TextStyle(
        color: Colors.white,
        fontSize: size * 0.55,
        fontWeight: FontWeight.w700,
      ),
    ),
    textDirection: TextDirection.ltr,
  )..layout();
  tp.paint(
    canvas,
    Offset((size - tp.width) / 2, (size - tp.height) / 2),
  );
  return recorder.endRecording().toImage(size, size);
}

/// Horizontal banner that mimics a cover-art crop: a duotone gradient
/// plus a small caption (track name / artist). 350×135 matches what
/// MediaListener.kt emits for real album art, so the head unit renders
/// it 1:1.
Future<ui.Image> _drawAlbumArtBitmap({
  required int width,
  required int height,
  required Color top,
  required Color bottom,
  required String caption,
}) async {
  final recorder = ui.PictureRecorder();
  final canvas = Canvas(recorder);
  final rect = Offset.zero & Size(width.toDouble(), height.toDouble());
  final paint = Paint()
    ..shader = ui.Gradient.linear(
      rect.topLeft,
      rect.bottomRight,
      [top, bottom],
    );
  canvas.drawRect(rect, paint);
  final tp = TextPainter(
    text: TextSpan(
      text: caption,
      style: const TextStyle(
        color: Colors.white,
        fontSize: 18,
        fontWeight: FontWeight.w700,
      ),
    ),
    textDirection: TextDirection.ltr,
  )..layout(maxWidth: width.toDouble() - 24);
  tp.paint(canvas, Offset(16, height - tp.height - 14));
  return recorder.endRecording().toImage(width, height);
}

Future<Uint8List> makeIconPng(Color color, String letter, {int size = 72}) async {
  final img = await _drawIconBitmap(
    size: size, background: color, letter: letter,
  );
  final data = await img.toByteData(format: ui.ImageByteFormat.png);
  return data!.buffer.asUint8List();
}

Future<Uint8List> makeAlbumArtPng({
  required String caption,
  required Color top,
  required Color bottom,
  int width = 350,
  int height = 135,
}) async {
  final img = await _drawAlbumArtBitmap(
    width: width, height: height,
    top: top, bottom: bottom,
    caption: caption,
  );
  final data = await img.toByteData(format: ui.ImageByteFormat.png);
  return data!.buffer.asUint8List();
}
