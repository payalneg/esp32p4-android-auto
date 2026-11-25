/// SLIP framing used by the ESP ROM serial bootloader.
///
/// Every packet is `C0 <escaped payload> C0`; `C0` inside the payload becomes
/// `DB DC`, `DB` becomes `DB DD`. Responses are SLIP frames too, so the same
/// decoder handles the reply direction.
library;

import 'dart:typed_data';

const int slipEnd = 0xC0;
const int slipEsc = 0xDB;
const int slipEscEnd = 0xDC;
const int slipEscEsc = 0xDD;

/// Wrap [payload] in SLIP delimiters, escaping special bytes.
Uint8List slipEncode(List<int> payload) {
  final out = BytesBuilder(copy: false);
  out.addByte(slipEnd);
  for (final b in payload) {
    if (b == slipEnd) {
      out
        ..addByte(slipEsc)
        ..addByte(slipEscEnd);
    } else if (b == slipEsc) {
      out
        ..addByte(slipEsc)
        ..addByte(slipEscEsc);
    } else {
      out.addByte(b);
    }
  }
  out.addByte(slipEnd);
  return out.takeBytes();
}

/// Incremental SLIP decoder: feed raw USB chunks, get whole packets back.
class SlipDecoder {
  final List<int> _frame = <int>[];
  bool _escaped = false;

  /// Decodes [chunk] and returns every complete frame it contained.
  List<Uint8List> add(List<int> chunk) {
    final frames = <Uint8List>[];
    for (final b in chunk) {
      if (b == slipEnd) {
        if (_frame.isNotEmpty) {
          frames.add(Uint8List.fromList(_frame));
          _frame.clear();
        }
        _escaped = false;
      } else if (b == slipEsc) {
        _escaped = true;
      } else {
        var value = b;
        if (_escaped) {
          value = b == slipEscEnd ? slipEnd : (b == slipEscEsc ? slipEsc : b);
          _escaped = false;
        }
        _frame.add(value);
      }
    }
    return frames;
  }

  /// Throw away a partial packet (e.g. after aborting a command).
  void reset() {
    _frame.clear();
    _escaped = false;
  }
}
