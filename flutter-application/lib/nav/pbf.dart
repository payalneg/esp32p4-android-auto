/// Reader for the OSM PBF format — the same regional extracts scripts/mapgen
/// feeds to pyosmium.
///
/// Why this exists: Overpass answers with about a megabyte of JSON per square
/// kilometre, so a whole province is tens of gigabytes and simply cannot be
/// asked for. The same province from Geofabrik is under two hundred megabytes,
/// because PBF is protobuf in zlib blobs. Nothing in Dart reads it, so this
/// does — only the parts a routing graph needs.
///
/// Layout, per https://wiki.openstreetmap.org/wiki/PBF_Format:
///
///     [int32 BE header length][BlobHeader][Blob] repeated
///     BlobHeader { 1: string type, 3: int32 datasize }
///     Blob       { 1: bytes raw, 2: int32 raw_size, 3: bytes zlib_data }
///     PrimitiveBlock { 1: StringTable, 2: PrimitiveGroup+,
///                      17: granularity, 19: lat_offset, 20: lon_offset }
///
/// Nodes come before the ways that reference them and a province has millions
/// of them, so the file is read twice: once for the ways worth keeping and the
/// node ids they name, once for those nodes' coordinates. Two passes over
/// compressed blobs cost less than holding every node in memory.
library;

import 'dart:convert';
import 'dart:io';
import 'dart:typed_data';

/// Minimal protobuf wire-format reader: enough for the handful of fields the
/// OSM schema uses, without a code generator.
class ProtoReader {
  ProtoReader(this.data, [this.pos = 0, int? end]) : end = end ?? data.length;

  final Uint8List data;
  int pos;
  final int end;

  bool get isDone => pos >= end;

  int readVarint() {
    var result = 0;
    var shift = 0;
    while (pos < end) {
      final byte = data[pos++];
      result |= (byte & 0x7F) << shift;
      if (byte & 0x80 == 0) return result;
      shift += 7;
      if (shift > 63) throw const FormatException('PBF: varint too long');
    }
    throw const FormatException('PBF: varint runs past the end');
  }

  /// Zig-zag decoding, used for every delta-encoded field in OSM.
  int readSVarint() {
    final v = readVarint();
    return (v >> 1) ^ -(v & 1);
  }

  /// (field number, wire type) of the next field.
  (int, int) readTag() {
    final tag = readVarint();
    return (tag >> 3, tag & 7);
  }

  Uint8List readBytes() {
    final length = readVarint();
    if (pos + length > end) {
      throw const FormatException('PBF: length-delimited field overruns');
    }
    final out = Uint8List.sublistView(data, pos, pos + length);
    pos += length;
    return out;
  }

  String readString() => utf8.decode(readBytes(), allowMalformed: true);

  /// Skips a field whose contents are not needed.
  void skip(int wireType) {
    switch (wireType) {
      case 0:
        readVarint();
      case 1:
        pos += 8;
      case 2:
        // Read the length first: `pos += readVarint()` would add it to the
        // position from *before* the varint was consumed, landing short by
        // the width of that varint.
        final length = readVarint();
        pos += length;
      case 5:
        pos += 4;
      default:
        throw FormatException('PBF: unknown wire type $wireType');
    }
  }
}

/// One decompressed primitive block, with its string table and scaling.
class PrimitiveBlock {
  PrimitiveBlock(this.bytes);

  final Uint8List bytes;
  final List<String> strings = <String>[];
  int granularity = 100;
  int latOffset = 0;
  int lonOffset = 0;
  final List<Uint8List> groups = <Uint8List>[];

  /// Reads the header fields and collects the groups; the groups themselves
  /// are decoded on demand, so a pass that only wants ways does not pay for
  /// the nodes.
  void parse() {
    final r = ProtoReader(bytes);
    while (!r.isDone) {
      final (field, wire) = r.readTag();
      switch (field) {
        case 1:
          _parseStringTable(r.readBytes());
        case 2:
          groups.add(r.readBytes());
        case 17:
          granularity = r.readVarint();
        case 19:
          latOffset = r.readVarint();
        case 20:
          lonOffset = r.readVarint();
        default:
          r.skip(wire);
      }
    }
  }

  void _parseStringTable(Uint8List raw) {
    final r = ProtoReader(raw);
    while (!r.isDone) {
      final (field, wire) = r.readTag();
      if (field == 1) {
        strings.add(r.readString());
      } else {
        r.skip(wire);
      }
    }
  }

  double lat(int delta) => (latOffset + granularity * delta) * 1e-9;
  double lon(int delta) => (lonOffset + granularity * delta) * 1e-9;
}

/// Walks the blobs of a PBF file, handing each decompressed block over.
class PbfFile {
  PbfFile(this.path);

  final String path;

  /// Calls [onBlock] for every OSMData block in file order.
  ///
  /// [onProgress] reports bytes consumed, so a two-pass build can show one
  /// honest bar rather than two mysterious ones.
  Future<void> forEachBlock(
    void Function(PrimitiveBlock block) onBlock, {
    void Function(int bytesRead, int total)? onProgress,
    bool Function()? cancelled,
  }) async {
    final file = File(path);
    final total = await file.length();
    final handle = await file.open();
    final zlib = ZLibCodec();
    try {
      var offset = 0;
      while (offset < total) {
        if (cancelled?.call() ?? false) return;
        await handle.setPosition(offset);
        final lengthBytes = await handle.read(4);
        if (lengthBytes.length < 4) break;
        final headerLength =
            ByteData.sublistView(lengthBytes).getUint32(0, Endian.big);
        final header = await handle.read(headerLength);
        final (type, dataSize) = _parseBlobHeader(header);
        final blobRaw = await handle.read(dataSize);
        offset += 4 + headerLength + dataSize;
        onProgress?.call(offset, total);
        if (type != 'OSMData') continue;
        final payload = _inflateBlob(blobRaw, zlib);
        final block = PrimitiveBlock(payload)..parse();
        onBlock(block);
      }
    } finally {
      await handle.close();
    }
  }

  static (String, int) _parseBlobHeader(Uint8List bytes) {
    final r = ProtoReader(bytes);
    var type = '';
    var size = 0;
    while (!r.isDone) {
      final (field, wire) = r.readTag();
      switch (field) {
        case 1:
          type = r.readString();
        case 3:
          size = r.readVarint();
        default:
          r.skip(wire);
      }
    }
    return (type, size);
  }

  static Uint8List _inflateBlob(Uint8List bytes, ZLibCodec zlib) {
    final r = ProtoReader(bytes);
    while (!r.isDone) {
      final (field, wire) = r.readTag();
      switch (field) {
        case 1: // stored uncompressed
          return r.readBytes();
        case 3: // zlib, the usual case
          final compressed = r.readBytes();
          final out = zlib.decode(compressed);
          return out is Uint8List ? out : Uint8List.fromList(out);
        default:
          r.skip(wire);
      }
    }
    throw const FormatException('PBF: blob has no data this reader can read');
  }
}
