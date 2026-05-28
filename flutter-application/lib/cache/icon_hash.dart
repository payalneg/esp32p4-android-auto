import 'dart:typed_data';

/// FNV-1a 32-bit. Stable hash for icon/album-art deduplication.
int fnv1a32(Uint8List bytes) {
  int h = 0x811c9dc5;
  for (final b in bytes) {
    h ^= b;
    h = (h * 0x01000193) & 0xffffffff;
  }
  return h;
}
