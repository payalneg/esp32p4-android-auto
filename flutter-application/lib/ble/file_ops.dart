/// Wire protocol for the device file manager over BLE — mirror of firmware
/// main/ble_files.c.
///
/// Two characteristics on the NotifBridge service:
///   charFileCtrl (...0009)  WRITE | NOTIFY  — request opcodes + status + bulk
///                                              listing/download payload
///   charFileData (...000a)  WRITE_NO_RSP    — raw upload bytes
library;

import 'dart:typed_data';

class FileOps {
  FileOps._();

  // CTRL request opcodes (phone → head unit). First byte of a CTRL write.
  static const opList = 0x01; // [TLV {1:path}]
  static const opDownload = 0x02; // [TLV {1:path}]
  static const opUpload = 0x03; // [u32 len][32B sha256][TLV {1:path}]
  static const opUpEnd = 0x04; // finalize the in-flight upload
  static const opDelete = 0x05; // [TLV {1:path}]
  static const opRename = 0x06; // [TLV {1:src}{2:dst}]
  static const opMkdir = 0x07; // [TLV {1:path}]
  static const opAbort = 0x08;

  // CTRL status notifications (head unit → phone): [u8 status][u32 detail LE].
  static const stReady = 0x10; // detail = size (download) / 0 (upload accepted)
  static const stProgress = 0x11; // detail = bytes received (upload)
  static const stDone = 0x12;
  static const stNotReady = 0x1e; // detail = app_fs_state() — storage mounting
  static const stError = 0x1f; // detail = errcode below

  // Bulk payload sub-tags (body[0] of a PDU_TYPE_FILE chunk message).
  static const subList = 0x01;
  static const subDownload = 0x02;

  // ERROR detail codes (match FERR_* in ble_files.c).
  static const errNotReady = 1;
  static const errBadPath = 2;
  static const errNoent = 3;
  static const errNotDir = 4;
  static const errIsDir = 5;
  static const errNoSpc = 6;
  static const errExist = 7;
  static const errIo = 8;
  static const errSha = 9;
  static const errProto = 10;
  static const errAlloc = 11;
  static const errBusy = 12;
  static const errTooBig = 13;

  /// i18n key (see lib/i18n/strings.dart) for a device-reported error code.
  static String errKey(int code) {
    switch (code) {
      case errNotReady:
        return 'files.err.notready';
      case errBadPath:
        return 'files.err.badpath';
      case errNoent:
        return 'files.err.noent';
      case errNotDir:
        return 'files.err.notdir';
      case errIsDir:
        return 'files.err.isdir';
      case errNoSpc:
        return 'files.err.nospc';
      case errExist:
        return 'files.err.exist';
      case errIo:
        return 'files.err.io';
      case errSha:
        return 'files.err.sha';
      case errProto:
        return 'files.err.proto';
      case errAlloc:
        return 'files.err.alloc';
      case errBusy:
        return 'files.err.busy';
      case errTooBig:
        return 'files.err.toobig';
      default:
        return 'files.err.unknown';
    }
  }
}

/// One entry in a directory listing.
class RemoteEntry {
  final String name;
  final bool isDir;
  final int size;
  final int mtime; // unix seconds, 0 if unknown

  const RemoteEntry({
    required this.name,
    required this.isDir,
    required this.size,
    required this.mtime,
  });
}

/// Decoded directory listing.
class RemoteListing {
  final List<RemoteEntry> entries;
  final bool truncated; // device hit its per-listing cap

  const RemoteListing(this.entries, this.truncated);

  /// Parse a SUB_LIST bulk body:
  ///   [u8 sub=0x01][u8 flags(bit0=truncated)][u16 count]
  ///   count× [u16 name_len][name][u8 is_dir][u32 size][u32 mtime]
  static RemoteListing parse(Uint8List body) {
    if (body.length < 4 || body[0] != FileOps.subList) {
      return const RemoteListing([], false);
    }
    final bd = ByteData.sublistView(body);
    final truncated = (body[1] & 0x01) != 0;
    final count = bd.getUint16(2, Endian.little);
    final entries = <RemoteEntry>[];
    int off = 4;
    for (int i = 0; i < count && off + 2 <= body.length; i++) {
      final nameLen = bd.getUint16(off, Endian.little);
      off += 2;
      if (off + nameLen + 9 > body.length) break;
      final name = String.fromCharCodes(body, off, off + nameLen);
      off += nameLen;
      final isDir = body[off] != 0;
      off += 1;
      final size = bd.getUint32(off, Endian.little);
      off += 4;
      final mtime = bd.getUint32(off, Endian.little);
      off += 4;
      entries.add(RemoteEntry(
          name: name, isDir: isDir, size: size, mtime: mtime));
    }
    return RemoteListing(entries, truncated);
  }
}

/// Thrown by FileManager ops on a device-reported error. [key] is an i18n key.
class FileOpException implements Exception {
  final String key;
  final bool notReady; // storage still mounting/formatting → caller can retry
  const FileOpException(this.key, {this.notReady = false});
  @override
  String toString() => 'FileOpException($key)';
}
