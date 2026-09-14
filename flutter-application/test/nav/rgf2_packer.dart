/// Test-only writer for the RGF2 format, mirroring `pack_graph` in
/// scripts/mapgen/graph_format.py. Having the encoder here means the parser is
/// tested against an independent implementation of the same spec rather than
/// against itself.
library;

import 'dart:typed_data';

class TestEdge {
  TestEdge({
    required this.from,
    required this.to,
    required this.lenDm,
    required this.wayClass,
    this.bidir = true,
    required this.points,
  });

  final int from;
  final int to;
  final int lenDm;
  final int wayClass;
  final bool bidir;

  /// (lat, lon) pairs including both endpoints.
  final List<(double, double)> points;
}

Uint8List packGraph(
  List<(double, double)> nodes,
  List<TestEdge> edges, {
  int magic = 0x32464752,
}) {
  var size = 12 + nodes.length * 8;
  for (final e in edges) {
    size += 16 + e.points.length * 8;
  }
  final out = Uint8List(size);
  final d = ByteData.sublistView(out);
  d.setUint32(0, magic, Endian.little);
  d.setUint32(4, nodes.length, Endian.little);
  d.setUint32(8, edges.length, Endian.little);
  var off = 12;
  for (final n in nodes) {
    d.setInt32(off, (n.$1 * 1e7).round(), Endian.little);
    d.setInt32(off + 4, (n.$2 * 1e7).round(), Endian.little);
    off += 8;
  }
  for (final e in edges) {
    d.setUint32(off, e.from, Endian.little);
    d.setUint32(off + 4, e.to, Endian.little);
    d.setUint32(off + 8, e.lenDm, Endian.little);
    d.setUint8(off + 12, e.wayClass);
    d.setUint8(off + 13, e.bidir ? 1 : 0);
    d.setUint16(off + 14, e.points.length, Endian.little);
    off += 16;
    for (final p in e.points) {
      d.setInt32(off, (p.$1 * 1e7).round(), Endian.little);
      d.setInt32(off + 4, (p.$2 * 1e7).round(), Endian.little);
      off += 8;
    }
  }
  return out;
}
