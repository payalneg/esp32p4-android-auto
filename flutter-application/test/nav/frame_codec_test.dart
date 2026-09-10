/// Encoding a rendered frame for the head unit's hardware JPEG decoder.
library;

import 'dart:typed_data';

import 'package:aa_bridge/nav/frame_codec.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:image/image.dart' as img;

RawFrame _frame(int w, int h, {int seed = 0}) {
  final rgba = Uint8List(w * h * 4);
  for (var i = 0; i < w * h; i++) {
    rgba[i * 4] = (i + seed) & 0xFF;
    rgba[i * 4 + 1] = (i ~/ w) & 0xFF;
    rgba[i * 4 + 2] = seed & 0xFF;
    rgba[i * 4 + 3] = 0xFF;
  }
  return RawFrame(w, h, rgba);
}

/// Reads the sampling factor of the first component out of the SOF0 marker.
/// 0x22 means 2x2 chroma subsampling, i.e. 4:2:0.
int _lumaSamplingFactor(Uint8List jpeg) {
  for (var i = 2; i + 9 < jpeg.length;) {
    if (jpeg[i] != 0xFF) {
      i++;
      continue;
    }
    final marker = jpeg[i + 1];
    final len = (jpeg[i + 2] << 8) | jpeg[i + 3];
    if (marker == 0xC0 || marker == 0xC1) {
      // [len:2][precision:1][h:2][w:2][ncomp:1][id:1][sampling:1]...
      return jpeg[i + 4 + 6 + 1];
    }
    if (marker == 0xD8 || marker == 0xD9) {
      i += 2;
      continue;
    }
    i += 2 + len;
  }
  return -1;
}

void main() {
  test('the frame size is whole macroblocks in the panel proportions', () {
    expect(kNavFrameW % 16, 0);
    expect(kNavFrameH % 16, 0);
    // 5:3, same as the head unit's 800x480 panel — it scales without
    // letterboxing and rejects anything else.
    expect(kNavFrameW * 480, kNavFrameH * 800);
  });

  test('a captured frame encodes to a JPEG of the same size', () {
    final jpeg = encodeNavFrame(_frame(kNavFrameW, kNavFrameH));
    final decoded = img.decodeJpg(jpeg);
    expect(decoded, isNotNull);
    expect(decoded!.width, kNavFrameW);
    expect(decoded.height, kNavFrameH);
  });

  test('the JPEG is 4:2:0, which is all the head unit can decode', () {
    // The P4's decoder hardwires a YUV420 -> RGB565 conversion; 4:4:4 (the
    // image package's default) comes out as chroma mush.
    final jpeg = encodeNavFrame(_frame(kNavFrameW, kNavFrameH));
    expect(_lumaSamplingFactor(jpeg), 0x22);
  });

  test('a size that is not whole macroblocks is cropped down', () {
    final jpeg = encodeNavFrame(_frame(404, 244));
    final decoded = img.decodeJpg(jpeg)!;
    expect(decoded.width, 400);
    expect(decoded.height, 240);
  });

  test('the hash separates frames and repeats for identical ones', () {
    final a = _frame(64, 48);
    final b = _frame(64, 48);
    final c = _frame(64, 48, seed: 7);
    expect(frameHash(a.rgba), frameHash(b.rgba));
    expect(frameHash(a.rgba), isNot(frameHash(c.rgba)));
  });

  test('one changed pixel changes the hash', () {
    final a = _frame(64, 48);
    final before = frameHash(a.rgba);
    a.rgba[4 * 1000] = a.rgba[4 * 1000] ^ 0xFF;
    expect(frameHash(a.rgba), isNot(before));
  });
}
