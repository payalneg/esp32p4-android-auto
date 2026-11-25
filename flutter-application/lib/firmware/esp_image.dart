/// Inspects an ESP firmware .bin picked for USB flashing.
///
/// Two shapes are recognised:
///
///  * **merged** — one file written at 0x0 that contains everything the chip
///    needs (bootloader at the chip's bootloader offset, partition table at
///    0x8000, application at its partition offset). This is what `esptool
///    merge_bin` / `release/flash.py` use, and the only thing that can revive
///    a head unit whose bootloader or partition table is damaged.
///  * **application** — the .bin produced by a normal build/OTA. It has to be
///    written at the application partition offset (0x20000 on this project's
///    P4 partition table), so the existing bootloader/partition table stay in
///    place.
///
/// The image header carries the chip id and the supported silicon revision
/// range. Reading them here — before anything is erased — is what keeps a v1.x
/// image off a v3.x P4 board (see docs/esp32p4-rev3.md).
library;

import 'dart:typed_data';

import 'esptool/esp_chip.dart';

enum EspImageKind { merged, application, unknown }

class EspImageInfo {
  const EspImageInfo._({
    required this.kind,
    required this.length,
    this.chipId,
    this.minRev = 0,
    this.maxRev = 0xFFFF,
    this.bootloaderOffset = 0,
    this.errorKey,
  });

  final EspImageKind kind;
  final int length;

  /// Chip id from the (bootloader or application) image header, when present.
  final int? chipId;
  final int minRev;
  final int maxRev;
  final int bootloaderOffset;

  /// i18n key describing why the file was rejected, or null when usable.
  final String? errorKey;

  EspChip? get chip => EspChip.byId(chipId);

  bool get isMerged => kind == EspImageKind.merged;
  bool get isApplication => kind == EspImageKind.application;
  bool get isValid => kind != EspImageKind.unknown && errorKey == null;

  /// Offset the whole file must be written at.
  ///
  /// Application-only images assume the partition table this device family
  /// uses: 0x20000 on this project's P4 boards, the ESP-IDF default 0x10000 on
  /// anything else. The UI lets the user override it for foreign images.
  int get flashOffset {
    if (isMerged) return 0;
    return chipId == EspChip.esp32p4.id ? 0x20000 : 0x10000;
  }

  /// Human-readable revision range (`v1.0 – v1.99`, `v3.0+`…).
  String get revisionRange {
    if (maxRev == 0 || maxRev == 0xFFFF) {
      return '${EspChip.revString(minRev)}+';
    }
    return '${EspChip.revString(minRev)}–${EspChip.revString(maxRev)}';
  }

  /// True when [chipRev] (esptool's major*100+minor) fits this image.
  bool acceptsRevision(int chipRev) {
    if (maxRev == 0 || maxRev == 0xFFFF) return chipRev >= minRev;
    return chipRev >= minRev && chipRev <= maxRev;
  }

  static const unknown = EspImageInfo._(
    kind: EspImageKind.unknown,
    length: 0,
    errorKey: 'fw.usb.file.invalid',
  );
}

const int _imageMagic = 0xE9;
const int _partitionMagic = 0xAA50;
const int _partitionTableOffset = 0x8000;
const int _headerSize = 24;

/// Ping-pong of the extended revision fields, exactly like esptool's
/// LoadFirmwareImage: min/max full revisions at bytes 15 and 17.
({int minRev, int maxRev}) readImageRevision(Uint8List data, int headerOffset) {
  if (headerOffset + _headerSize > data.length) {
    return (minRev: 0, maxRev: 0xFFFF);
  }
  final bd = ByteData.sublistView(data, headerOffset, headerOffset + _headerSize);
  return (minRev: bd.getUint16(15, Endian.little), maxRev: bd.getUint16(17, Endian.little));
}

int? readImageChipId(Uint8List data, int headerOffset) {
  if (headerOffset + _headerSize > data.length || data[headerOffset] != _imageMagic) {
    return null;
  }
  return ByteData.sublistView(data, headerOffset + 12, headerOffset + 14)
      .getUint16(0, Endian.little);
}

/// A partition table starts with 0xAA50 entries, each 32 bytes. Returns the
/// offset of the first application partition (type 0x00), or null when the
/// table is absent/invalid.
int? _firstAppOffset(Uint8List data) {
  const tableOffset = _partitionTableOffset;
  const maxEntries = 95; // 0xC00 bytes, the partition table limit
  for (var i = 0; i < maxEntries; i++) {
    final entry = tableOffset + i * 32;
    if (entry + 32 > data.length) return null;
    // 0xFFFF marks the end of the table (and matches unwritten flash).
    if (data[entry] == 0xFF && data[entry + 1] == 0xFF) return null;
    final magic = (data[entry] << 8) | data[entry + 1];
    if (magic != _partitionMagic) return null;
    final type = data[entry + 2];
    final addr = ByteData.sublistView(data, entry + 4, entry + 8)
        .getUint32(0, Endian.little);
    final size = ByteData.sublistView(data, entry + 8, entry + 12)
        .getUint32(0, Endian.little);
    if (addr + size > 0x2000000 || size == 0) return null;
    if (type == 0x00) return addr;
  }
  return null;
}

/// A usable merged image must actually contain the application at the offset
/// its partition table points to; a bootloader-only file is not flashable.
bool _containsApp(Uint8List data, int? appOffset) =>
    appOffset != null &&
    appOffset + _headerSize <= data.length &&
    data[appOffset] == _imageMagic;

EspImageInfo inspectEspImage(Uint8List data) {
  if (data.length < _headerSize || data[0] != _imageMagic) {
    // A P4 merged image is padded with 0xFF up to the bootloader at 0x2000.
    if (data.length > 0x2024 && data[0x2000] == _imageMagic) {
      final chipId = readImageChipId(data, 0x2000);
      final rev = readImageRevision(data, 0x2000);
      final appOffset = _firstAppOffset(data);
      final appChip = appOffset == null ? null : readImageChipId(data, appOffset);
      final valid = chipId == EspChip.esp32p4.id &&
          _containsApp(data, appOffset) &&
          appChip == chipId;
      return EspImageInfo._(
        kind: valid ? EspImageKind.merged : EspImageKind.unknown,
        length: data.length,
        chipId: chipId,
        minRev: rev.minRev,
        maxRev: rev.maxRev,
        bootloaderOffset: 0x2000,
        errorKey: valid ? null : 'fw.usb.file.invalid',
      );
    }
    return EspImageInfo.unknown;
  }

  final chipId = readImageChipId(data, 0);
  final rev = readImageRevision(data, 0);
  final appOffset = _firstAppOffset(data);
  final appChip = appOffset == null ? null : readImageChipId(data, appOffset);
  if (_containsApp(data, appOffset) && appChip == chipId) {
    return EspImageInfo._(
      kind: EspImageKind.merged,
      length: data.length,
      chipId: chipId,
      minRev: rev.minRev,
      maxRev: rev.maxRev,
      bootloaderOffset: 0,
    );
  }
  return EspImageInfo._(
    kind: EspImageKind.application,
    length: data.length,
    chipId: chipId,
    minRev: rev.minRev,
    maxRev: rev.maxRev,
  );
}
