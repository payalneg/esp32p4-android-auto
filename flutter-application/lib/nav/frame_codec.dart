/// Turning a rendered navigator view into the bytes the head unit can decode.
///
/// The head unit has a hardware JPEG decoder with two rules we have to respect
/// (see main/ble_nav.h): the picture must be **4:2:0**, because the RGB565
/// output path hardwires a YUV420 colour conversion and turns 4:4:4 into
/// chroma mush, and both axes must be whole 16-pixel macroblocks.
///
/// Encoding is pure top-level Dart so it can run in a `compute` isolate — a
/// 400x240 JPEG takes long enough to drop frames on the UI thread.
library;

import 'dart:typed_data';

import 'package:image/image.dart' as img;

/// Frame size we render and send.
///
/// Half the panel in each axis: the head unit upscales with its PPA, and a
/// quarter of the pixels is what makes one frame a second possible at all on a
/// link that carries tens of kilobytes a second. Both axes are multiples of 16
/// and the 5:3 ratio matches the 800x480 panel, so the picture is never
/// letterboxed or stretched.
const int kNavFrameW = 400;
const int kNavFrameH = 240;

/// JPEG quality for a frame. Map tiles are flat colour with thin lines, which
/// survives 80 well; the frames land around 15-25 KB.
const int kNavJpegQuality = 80;

/// A captured frame, straight out of `Image.toByteData(rawRgba)`.
class RawFrame {
  const RawFrame(this.width, this.height, this.rgba);

  final int width;
  final int height;
  final Uint8List rgba;
}

/// Encodes [frame] as a baseline 4:2:0 JPEG.
///
/// Top-level and self-contained so `compute` can run it. Sizes that are not
/// whole macroblocks are cropped down to the nearest one rather than padded:
/// a stray row is invisible, an oversized frame is rejected by the head unit.
Uint8List encodeNavFrame(RawFrame frame) {
  final w = frame.width - (frame.width % 16);
  final h = frame.height - (frame.height % 16);
  var image = img.Image.fromBytes(
    width: frame.width,
    height: frame.height,
    bytes: frame.rgba.buffer,
    bytesOffset: frame.rgba.offsetInBytes,
    numChannels: 4,
    order: img.ChannelOrder.rgba,
  );
  if (w != frame.width || h != frame.height) {
    image = img.copyCrop(image, x: 0, y: 0, width: w, height: h);
  }
  return img.encodeJpg(image,
      quality: kNavJpegQuality, chroma: img.JpegChroma.yuv420);
}

/// A cheap content hash, so a picture that did not change is not sent again.
///
/// A parked bike would otherwise spend the whole stop pushing identical
/// frames. FNV-1a over the raw pixels, read a word at a time — fast enough to
/// run on every capture.
int frameHash(Uint8List rgba) {
  final words = Uint32List.sublistView(
      rgba, 0, rgba.lengthInBytes - (rgba.lengthInBytes % 4));
  var h = 0x811c9dc5;
  for (var i = 0; i < words.length; i++) {
    h = (h ^ words[i]) & 0xFFFFFFFF;
    h = (h * 0x01000193) & 0xFFFFFFFF;
  }
  return h;
}
