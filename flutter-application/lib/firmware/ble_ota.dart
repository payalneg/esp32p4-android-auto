/// Wire protocol for flashing the head unit's firmware straight over the BLE
/// link — mirror of firmware main/ble_ota.c.
///
/// Two characteristics on the NotifBridge service:
///   charOtaCtrl (...0007)  WRITE | NOTIFY  — control + status
///   charOtaData (...0008)  WRITE_NO_RSP    — raw firmware bytes
///
/// Flow: phone writes BEGIN (size + sha256) on CTRL, waits for the READY
/// notification, streams the image on DATA, writes END, then waits for DONE
/// (or ERROR). The head unit stages the image in PSRAM, verifies the sha256,
/// flashes the spare OTA slot and reboots.
library;

class BleOta {
  BleOta._();

  // CTRL opcodes (phone → head unit).
  static const opBegin = 0x01;
  static const opEnd = 0x02;
  static const opAbort = 0x03;

  // CTRL status notifications (head unit → phone): [u8 status][u32 detail LE].
  // READY's detail is the largest DATA write the firmware accepts (509 on
  // current builds); 0 means an older firmware whose buffer fits 244 bytes.
  static const stReady = 0x10;
  static const stProgress = 0x11;
  static const stDone = 0x12;
  static const stError = 0x1f;

  // ERROR detail codes (match OTA_ERR_* in ble_ota.c).
  static const errNoPart = 1;
  static const errSize = 2;
  static const errAlloc = 3;
  static const errSha = 4;
  static const errBegin = 5;
  static const errWrite = 6;
  static const errEnd = 7;
  static const errBoot = 8;
  static const errProto = 9;

  /// i18n key (see lib/i18n/strings.dart) for a device-reported error code.
  /// Returns a key so callers without a BuildContext stay locale-agnostic; the
  /// UI maps it to text via `t()`.
  static String errKey(int code) {
    switch (code) {
      case errNoPart:
        return 'fw.ota.err.nopart';
      case errSize:
        return 'fw.ota.err.size';
      case errAlloc:
        return 'fw.ota.err.alloc';
      case errSha:
        return 'fw.ota.err.sha';
      case errBegin:
        return 'fw.ota.err.begin';
      case errWrite:
        return 'fw.ota.err.write';
      case errEnd:
        return 'fw.ota.err.end';
      case errBoot:
        return 'fw.ota.err.boot';
      case errProto:
        return 'fw.ota.err.proto';
      default:
        return 'fw.ota.err.unknown';
    }
  }
}
