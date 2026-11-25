/// Minimal ESP chip database for the USB flasher.
///
/// Only the fields the ROM loader needs are modelled: the chip id stored in an
/// image header, the bootloader flash offset and whether the ROM supports the
/// extra "encrypted flash" argument of FLASH_BEGIN.
library;

class EspChip {
  const EspChip._(this.id, this.name, this.bootloaderOffset, this.encryptedFlash);

  final int id;
  final String name;
  final int bootloaderOffset;
  final bool encryptedFlash;

  static const esp8266 = EspChip._(-1, 'ESP8266', 0x0, false);
  static const esp32 = EspChip._(0, 'ESP32', 0x1000, false);
  static const esp32s2 = EspChip._(2, 'ESP32-S2', 0x1000, true);
  static const esp32c3 = EspChip._(5, 'ESP32-C3', 0x0, true);
  static const esp32s3 = EspChip._(9, 'ESP32-S3', 0x0, true);
  static const esp32c2 = EspChip._(12, 'ESP32-C2', 0x0, true);
  static const esp32c6 = EspChip._(13, 'ESP32-C6', 0x0, true);
  static const esp32h2 = EspChip._(16, 'ESP32-H2', 0x0, true);
  static const esp32p4 = EspChip._(18, 'ESP32-P4', 0x2000, true);

  static const all = <int, EspChip>{
    0: esp32,
    2: esp32s2,
    5: esp32c3,
    9: esp32s3,
    12: esp32c2,
    13: esp32c6,
    16: esp32h2,
    18: esp32p4,
  };

  /// Returns the chip for a ROM-reported image chip id, or null when unknown.
  static EspChip? byId(int? id) => id == null ? null : all[id];

  /// `v3.1` style revision string from esptool's `major * 100 + minor`.
  static String revString(int rev) => 'v${rev ~/ 100}.${rev % 100}';

  @override
  String toString() => name;
}
