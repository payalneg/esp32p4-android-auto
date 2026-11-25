/// A small, dependency-free implementation of the Espressif ROM serial
/// bootloader protocol (the one esptool speaks), enough to write flash from an
/// Android phone over USB.
///
/// Deliberately uses the ROM loader only — no flasher stub — so there is no
/// second binary to ship and no RAM download step. That costs speed (the ROM
/// does not compress) which is why the link is switched to 921600 Bd after
/// connecting on real UART bridges.
library;

import 'dart:async';
import 'dart:collection';
import 'dart:typed_data';

import 'package:crypto/crypto.dart' show md5;

import 'esp_chip.dart';
import 'slip.dart';

/// Raw serial link the loader talks through (USB CDC / CH343 / …).
abstract class EspTransport {
  Future<void> write(Uint8List data);

  /// Next bytes received, or null once [timeout] elapses.
  Future<Uint8List?> readChunk(Duration timeout);

  /// Drop everything buffered so far (both directions).
  Future<void> flushInput();

  Future<void> setBaud(int baud);
  Future<void> setDtr(bool value);
  Future<void> setRts(bool value);
  Future<void> close();
}

class EspException implements Exception {
  EspException(this.message);

  final String message;

  @override
  String toString() => message;
}

class EspTimeoutException extends EspException {
  EspTimeoutException(super.message);
}

class EspCommandException extends EspException {
  EspCommandException(int op, this.status, this.reason)
      : super('Command 0x${op.toRadixString(16)} failed '
            '(status 0x${status.toRadixString(16)}, '
            'reason 0x${reason.toRadixString(16)})');

  final int status;
  final int reason;
}

/// What the ROM loader told us about the chip it is running on.
class EspDeviceInfo {
  const EspDeviceInfo({
    required this.chipId,
    required this.chip,
    required this.revision,
    required this.usbJtagSerial,
    required this.usbOtg,
  });

  final int? chipId;
  final EspChip? chip;
  final int revision;
  final bool usbJtagSerial;
  final bool usbOtg;

  String get name => chip?.name ?? 'ESP32 (unknown)';
}

class _Reply {
  const _Reply(this.value, this.data);
  final int value;
  final Uint8List data;
}

/// One ROM command. `value` is the header value (e.g. READ_REG's result) and
/// `data` includes the two status bytes the ROM appends.
class EspLoader {
  EspLoader(this._transport);

  final EspTransport _transport;
  final SlipDecoder _decoder = SlipDecoder();
  final Queue<Uint8List> _frames = Queue<Uint8List>();

  /// Set by [connect]; decides which reset sequence recovers the link.
  bool _nativeUsb = false;

  /// Rate the transport and the ROM agree on. The ROM mirrors CHANGE_BAUD and
  /// keeps it until reset, so this is the only record of what it is talking.
  int _baud = 115200;

  /// Result of the last [connect], used to restore chip state after a reset.
  EspDeviceInfo? _info;

  static const opFlashBegin = 0x02;
  static const opFlashData = 0x03;
  static const opFlashEnd = 0x04;
  static const opSync = 0x08;
  static const opWriteReg = 0x09;
  static const opReadReg = 0x0A;
  static const opSpiSetParams = 0x0B;
  static const opSpiAttach = 0x0D;
  static const opChangeBaud = 0x0F;
  static const opFlashMd5 = 0x13;
  static const opSecurityInfo = 0x14;

  static const _checksumMagic = 0xEF;

  /// ROM flash block size. 0x400 is what the ROM loader accepts (esptool's
  /// `ESPLoader.FLASH_WRITE_SIZE`); 0x4000 is the *stub* loader's maximum and
  /// the ROM rejects it with INVALID_COMMAND (status 0x1 / reason 0x5,
  /// "parameters or length field is invalid") as soon as FLASH_BEGIN is sent.
  static const flashBlockSize = 0x400;

  /// SHA-256 of the image has nothing to do with the ROM checksum: it is the
  /// byte-wise XOR esptool calls "checksum", seeded with 0xEF.
  static int checksum(List<int> data, [int state = _checksumMagic]) {
    var s = state;
    for (final b in data) {
      s ^= b;
    }
    return s;
  }

  Future<void> flushInput() async {
    await _transport.flushInput();
    _frames.clear();
    _decoder.reset();
  }

  // ---------------------------------------------------------------- transport

  Future<Uint8List?> _nextFrame(Duration timeout) async {
    if (_frames.isNotEmpty) return _frames.removeFirst();
    final deadline = DateTime.now().add(timeout);
    while (true) {
      final left = deadline.difference(DateTime.now());
      if (left <= Duration.zero) return null;
      final chunk = await _transport.readChunk(left);
      if (chunk == null) return _frames.isNotEmpty ? _frames.removeFirst() : null;
      for (final frame in _decoder.add(chunk)) {
        _frames.add(frame);
      }
      if (_frames.isNotEmpty) return _frames.removeFirst();
    }
  }

  Future<_Reply?> _readReply(int? op, Duration timeout) async {
    final deadline = DateTime.now().add(timeout);
    while (true) {
      final left = deadline.difference(DateTime.now());
      if (left <= Duration.zero) return null;
      final frame = await _nextFrame(left);
      if (frame == null) return null;
      if (frame.length < 8 || frame[0] != 0x01) continue;
      if (op != null && frame[1] != op) continue;
      final bd = ByteData.sublistView(frame);
      return _Reply(
        bd.getUint32(4, Endian.little),
        Uint8List.sublistView(frame, 8),
      );
    }
  }

  Future<_Reply> _command(
    int? op, {
    List<int> data = const <int>[],
    int chk = 0,
    Duration timeout = const Duration(seconds: 3),
  }) async {
    if (op != null) {
      final header = ByteData(8)
        ..setUint8(0, 0x00)
        ..setUint8(1, op)
        ..setUint16(2, data.length, Endian.little)
        ..setUint32(4, chk, Endian.little);
      final packet = BytesBuilder(copy: false)
        ..add(header.buffer.asUint8List())
        ..add(data);
      await _transport.write(slipEncode(packet.takeBytes()));
    }
    final reply = await _readReply(op, timeout);
    if (reply == null) {
      throw EspTimeoutException(
          op == null ? 'No reply from the chip' : 'Command 0x${op.toRadixString(16)} timed out');
    }
    return reply;
  }

  /// Runs a command and turns the ROM status bytes into an exception.
  /// Returns the expected payload (without the status bytes).
  Future<Uint8List> _check(
    int op, {
    List<int> data = const <int>[],
    int chk = 0,
    int expectLen = 0,
    Duration timeout = const Duration(seconds: 3),
  }) async {
    final reply = await _command(op, data: data, chk: chk, timeout: timeout);
    final payload = reply.data;
    final statusAt = payload.length >= expectLen + 2 ? expectLen : 0;
    if (payload.length < statusAt + 2) {
      throw EspException('Short reply to command 0x${op.toRadixString(16)}');
    }
    if (payload[statusAt] != 0) {
      throw EspCommandException(op, payload[statusAt], payload[statusAt + 1]);
    }
    if (expectLen == 0) return Uint8List(0);
    if (payload.length < expectLen) {
      throw EspException('Short reply to command 0x${op.toRadixString(16)}');
    }
    return Uint8List.sublistView(payload, 0, expectLen);
  }

  static Uint8List _u32(int value) =>
      (ByteData(4)..setUint32(0, value, Endian.little)).buffer.asUint8List();

  // ------------------------------------------------------------------ control

  /// Best-effort SYNC. The ROM answers a SYNC with eight replies; the extras
  /// are drained so they cannot be mistaken for the next command's reply.
  Future<bool> trySync() async {
    await flushInput();
    try {
      await _command(
        opSync,
        data: <int>[0x07, 0x07, 0x12, 0x20, ...List<int>.filled(32, 0x55)],
        timeout: const Duration(milliseconds: 400),
      );
    } on EspException {
      return false;
    }
    for (var i = 0; i < 7; i++) {
      try {
        await _command(null, timeout: const Duration(milliseconds: 120));
      } on EspException {
        break;
      }
    }
    return true;
  }

  /// Pulse EN (RTS) with IO0 (DTR) high: a plain hard reset that boots the
  /// application instead of the ROM loader. Used after a write, because the
  /// ROM's own reboot request (FLASH_END) is ignored on USB-Serial/JTAG often
  /// enough that the board just sits in the loader — "flashes but never
  /// restarts".
  Future<void> hardReset() async {
    Future<void> safe(Future<void> Function() f) async {
      try {
        await f();
      } catch (_) {}
    }

    await safe(() => _transport.setDtr(false)); // IO0 high — boot the app
    await safe(() => _transport.setRts(true)); // EN low
    await Future<void>.delayed(const Duration(milliseconds: 150));
    await safe(() => _transport.setRts(false)); // EN high — out of reset
    await Future<void>.delayed(const Duration(milliseconds: 50));
  }

  /// Classic DTR/RTS auto-reset as wired on ESP dev boards.
  Future<void> classicReset() async {
    Future<void> safe(Future<void> Function() f) async {
      try {
        await f();
      } catch (_) {
        // Adapters without modem control still have the BOOT button.
      }
    }

    await safe(() => _transport.setDtr(false)); // IO0 high
    await safe(() => _transport.setRts(true)); // EN low, chip in reset
    await Future<void>.delayed(const Duration(milliseconds: 120));
    await safe(() => _transport.setDtr(true)); // IO0 low
    await safe(() => _transport.setRts(false)); // EN high, out of reset
    await Future<void>.delayed(const Duration(milliseconds: 60));
    await safe(() => _transport.setDtr(false)); // IO0 high, bootloader runs
  }

  /// Reset sequence the USB-Serial/JTAG and USB-OTG peripherals expect.
  Future<void> usbReset() async {
    Future<void> safe(Future<void> Function() f) async {
      try {
        await f();
      } catch (_) {}
    }

    await safe(() => _transport.setRts(false));
    await safe(() => _transport.setDtr(false));
    await Future<void>.delayed(const Duration(milliseconds: 120));
    await safe(() => _transport.setDtr(true)); // hold IO0
    await safe(() => _transport.setRts(false));
    await Future<void>.delayed(const Duration(milliseconds: 120));
    await safe(() => _transport.setRts(true)); // reset
    await safe(() => _transport.setDtr(false));
    await safe(() => _transport.setRts(true));
    await Future<void>.delayed(const Duration(milliseconds: 120));
    await safe(() => _transport.setDtr(false));
    await safe(() => _transport.setRts(false));
  }

  /// Connect to the ROM loader, resetting the chip if it is running an app.
  /// [nativeUsb] selects the USB-JTAG reset sequence (Espressif VID 0x303A).
  Future<EspDeviceInfo> connect({required bool nativeUsb}) async {
    _nativeUsb = nativeUsb;
    var synced = false;

    /// Rates a previous attempt may have left the chip talking at: the ROM
    /// keeps CHANGE_BAUD until it is reset, so a flash that died after the
    /// switch leaves a loader that ignores 115200. Trying the rates we ever
    /// set turns "stuck until you unplug it" into "reconnects".
    const rates = <int>[115200, 460800, 921600];
    String? lastError;

    for (var attempt = 0; attempt < 4 && !synced; attempt++) {
      if (attempt > 0) {
        if (nativeUsb) {
          await usbReset();
        } else {
          await classicReset();
        }
        await Future<void>.delayed(const Duration(milliseconds: 250));
      }
      for (final rate in rates) {
        if (rate != 115200) {
          try {
            await _transport.setBaud(rate);
          } catch (_) {
            continue;
          }
          await Future<void>.delayed(const Duration(milliseconds: 60));
        }
        // A bulk write can fail outright (a USB-Serial/JTAG left in a bad
        // state answers nothing and the transfer times out) — that must not
        // abort the run before the reset sequences have had their chance.
        var ok = false;
        try {
          ok = await trySync();
        } catch (e) {
          lastError = e.toString();
        }
        if (ok) {
          _baud = rate;
          synced = true;
          if (rate != 115200) {
            // Say it out loud: the log is how a "stuck" unit gets diagnosed.
            // ignore: avoid_print
            print('[usbflash] ROM answered at $rate Bd');
          }
          break;
        }
        if (rate != 115200) {
          try {
            await _transport.setBaud(115200);
          } catch (_) {}
        }
      }
    }
    if (!synced) {
      throw EspTimeoutException('No SYNC reply from the ROM loader'
          '${lastError == null ? '' : ' ($lastError)'}');
    }

    int? chipId;
    try {
      final info = await _check(opSecurityInfo,
          expectLen: 20, timeout: const Duration(seconds: 2));
      chipId = ByteData.sublistView(info, 12, 16).getUint32(0, Endian.little);
    } on EspException {
      // GET_SECURITY_INFO is ESP32-S2+ only; older chips still flash.
      await flushInput();
    }

    final chip = EspChip.byId(chipId);
    var revision = 0;
    var usbJtag = false;
    var usbOtg = false;

    if (chip?.id == EspChip.esp32p4.id) {
      revision = await _p4Revision();
      final uartNo = await _readUartNo(revision);
      usbJtag = uartNo == 6;
      usbOtg = uartNo == 5;
      if (usbJtag) {
        await _disableWatchdogs();
      }
      // The 3.x silicon powers the flash down by default (the default flash
      // voltage changed to 3.3 V); 1.x does not. esptool gates this on the
      // single ECO6 revision, but v3.2 needs it too, so use the whole 3.x
      // range.
      if (revision >= 300) {
        await _powerOnFlash();
      }
    }

    final device = EspDeviceInfo(
      chipId: chipId,
      chip: chip,
      revision: revision,
      usbJtagSerial: usbJtag,
      usbOtg: usbOtg,
    );
    _info = device;
    return device;
  }

  Future<int> _p4Revision() async {
    final reg = await readReg(0x5012D04C); // EFUSE_BLOCK1 + 2 words
    return ((reg >> 4) & 0x03) * 100 + (reg & 0x0F);
  }

  Future<int> _readUartNo(int revision) async {
    final addr = revision < 300 ? 0x4FF3FEB0 + 24 : 0x4FFBFEB0 + 24;
    try {
      return (await readReg(addr)) & 0xFF;
    } on EspException {
      return -1;
    }
  }

  /// USB-JTAG keeps the RTC/LP watchdogs running while the ROM is idle; left
  /// alone they reset the board in the middle of a long flash write.
  Future<void> _disableWatchdogs() async {
    const base = 0x50116000;
    try {
      await writeReg(base + 0x18, 0x50D83AA1);
      await writeReg(base + 0x00, 0);
      await writeReg(base + 0x18, 0);
      await writeReg(base + 0x20, 0x50D83AA1);
      await writeReg(base + 0x1C, (await readReg(base + 0x1C)) | (1 << 18));
      await writeReg(base + 0x20, 0);
    } on EspException {
      // Non-fatal: most boards still survive a flash.
    }
  }

  /// ESP32-P4 ECO6 powers the flash chip down by default; the ROM needs it
  /// powered before SPI_ATTACH/flash commands (esptool does the same).
  Future<void> _powerOnFlash() async {
    const padGroup = 0x5011010C;
    const pmuAna = 0x501151BC;
    const pmuLdo = 0x501151B8;
    const pmuDate = 0x501153FC;

    await writeReg(padGroup, 1);
    await Future<void>.delayed(const Duration(milliseconds: 10));
    await writeReg(pmuAna, (await readReg(pmuAna)) | (1 << 27));
    await writeReg(pmuLdo, (await readReg(pmuLdo)) | (1 << 7));
    await writeReg(pmuDate, (await readReg(pmuDate)) | 3);
    await Future<void>.delayed(const Duration(milliseconds: 1));
    await writeReg(pmuAna, (await readReg(pmuAna)) & ~(1 << 27));
    await writeReg(pmuLdo, (await readReg(pmuLdo)) & ~(0xFF << 23));
    await writeReg(pmuLdo, (await readReg(pmuLdo)) | 0x80);
    await writeReg(pmuLdo, (await readReg(pmuLdo)) & ~(1 << 7));
    await Future<void>.delayed(const Duration(milliseconds: 2));
  }

  Future<int> readReg(int addr) async {
    final reply =
        await _command(opReadReg, data: _u32(addr));
    final payload = reply.data;
    if (payload.length >= 2 && payload[0] != 0) {
      throw EspCommandException(opReadReg, payload[0], payload[1]);
    }
    return reply.value;
  }

  Future<void> writeReg(int addr, int value,
      {int mask = 0xFFFFFFFF, int delayUs = 0}) async {
    final data = BytesBuilder(copy: false)
      ..add(_u32(addr))
      ..add(_u32(value))
      ..add(_u32(mask))
      ..add(_u32(delayUs));
    await _check(opWriteReg, data: data.takeBytes());
  }

  /// Tell the ROM which SPI flash pins to use (0 = board default).
  Future<void> attachFlash() async {
    final data = <int>[0, 0, 0, 0, /* is_legacy */ 0, 0, 0, 0];
    await _check(opSpiAttach, data: data);
  }

  // ---- flash chip housekeeping -------------------------------------------
  //
  // esptool resets the SPI flash part before it starts writing, and without it
  // the ROM's FLASH_BEGIN fails with COMMAND_FAILED (status 1 / reason 6) on a
  // board that was running an app: the application (and the memory-mapped
  // reader) leaves the chip in a QIO/deep-power-down state that the very simple
  // ROM code cannot get out of. "Reset enable" (0x66) then "Reset" (0x99) is
  // the datasheet sequence, run through the SPI peripheral's USR_COMMAND path
  // — the same register pokes esptool's run_spiflash_command() does.

  static const _spiBase = 0x5008D000; // SPIMEM1 on the P4
  static const _spiCmdReg = _spiBase + 0x00;
  static const _spiAddrReg = _spiBase + 0x04;
  static const _spiUsrReg = _spiBase + 0x18;
  static const _spiUsr1Reg = _spiBase + 0x1C;
  static const _spiUsr2Reg = _spiBase + 0x20;
  static const _spiMosiDlenReg = _spiBase + 0x24;
  static const _spiMisoDlenReg = _spiBase + 0x28;
  static const _spiW0Reg = _spiBase + 0x58;

  static const _spiUsrCommand = 1 << 31;
  static const _spiUsrAddr = 1 << 30;
  static const _spiUsrDummy = 1 << 29;
  static const _spiUsrMiso = 1 << 28;
  static const _spiUsrMosi = 1 << 27;
  static const _spiCmdUsr = 1 << 18;
  static const _spiUsr2CommandLenShift = 28;
  static const _spiUsrAddrLenShift = 26;

  /// Read the flash chip's manufacturer/device id (0x9F), 24 bits.
  /// Returns 0 when the chip does not answer.
  Future<int> flashId() async {
    try {
      return await runSpiFlashCommand(0x9F, readBits: 24);
    } on EspException {
      return 0;
    }
  }

  /// SPI_SET_PARAMS (0x0B): fill the ROM's flashchip struct. Without it the ROM
  /// believes the chip has size 0, and the erase that FLASH_BEGIN asks for
  /// fails instantly with COMMAND_FAILED (status 1 / reason 6) — which is what
  /// "Command 0x2 failed" after 2 ms means. esptool sends the same six words
  /// before every no-stub write, with the flash size from --flash-size.
  Future<void> setFlashParams(int totalSize, {int statusMask = 0xFFFF}) async {
    final data = <int>[
      ..._u32(0), // fl_id: 0 — the ROM identifies the chip itself
      ..._u32(totalSize),
      ..._u32(64 * 1024), // block size
      ..._u32(4 * 1024), // sector size
      ..._u32(256), // page size
      ..._u32(statusMask),
    ];
    await _check(opSpiSetParams, data: data);
  }

  /// Run one arbitrary SPI flash command through the SPI peripheral's
  /// USR_COMMAND path. `addr`, `dummyLen` and `data` mirror esptool; the reset
  /// sequence needs none of them.
  Future<int> runSpiFlashCommand(
    int command, {
    List<int> data = const <int>[],
    int readBits = 0,
    int? addr,
    int addrLen = 0,
    int dummyLen = 0,
  }) async {
    final oldUsr = await readReg(_spiUsrReg);
    final oldUsr2 = await readReg(_spiUsr2Reg);

    final dataBits = data.length * 8;
    var flags = _spiUsrCommand;
    if (readBits > 0) flags |= _spiUsrMiso;
    if (dataBits > 0) flags |= _spiUsrMosi;
    if (addrLen > 0) flags |= _spiUsrAddr;
    if (dummyLen > 0) flags |= _spiUsrDummy;

    final usr1 = (dummyLen > 0 ? dummyLen - 1 : 0) |
        (addrLen > 0 ? (addrLen - 1) << _spiUsrAddrLenShift : 0);
    if (dataBits > 0) await writeReg(_spiMosiDlenReg, dataBits - 1);
    if (readBits > 0) await writeReg(_spiMisoDlenReg, readBits - 1);
    if (usr1 != 0) await writeReg(_spiUsr1Reg, usr1);

    await writeReg(_spiUsrReg, flags);
    await writeReg(_spiUsr2Reg, (7 << _spiUsr2CommandLenShift) | command);
    if (addrLen > 0) await writeReg(_spiAddrReg, addr! << (32 - addrLen));
    if (dataBits == 0) {
      await writeReg(_spiW0Reg, 0);
    } else {
      final padded = <int>[...data];
      while (padded.length % 4 != 0) {
        padded.add(0);
      }
      for (var i = 0; i < padded.length; i += 4) {
        final word = padded[i] |
            (padded[i + 1] << 8) |
            (padded[i + 2] << 16) |
            (padded[i + 3] << 24);
        await writeReg(_spiW0Reg + i, word);
      }
    }

    await writeReg(_spiCmdReg, _spiCmdUsr);
    var done = false;
    for (var i = 0; i < 10; i++) {
      if (((await readReg(_spiCmdReg)) & _spiCmdUsr) == 0) {
        done = true;
        break;
      }
    }
    final status = await readReg(_spiW0Reg);

    await writeReg(_spiUsrReg, oldUsr);
    await writeReg(_spiUsr2Reg, oldUsr2);
    if (!done) {
      throw EspException(
          'SPI command 0x${command.toRadixString(16)} did not complete');
    }
    return status;
  }

  /// Reset the flash chip's state machine — 0x66 (reset enable) + 0x99 (reset).
  /// Best-effort: a chip that does not answer leaves the flash as it was.
  Future<void> resetFlashChip() async {
    try {
      await runSpiFlashCommand(0x66);
      await runSpiFlashCommand(0x99);
      await Future<void>.delayed(const Duration(milliseconds: 2));
    } on EspException {
      // Try anyway: the ROM may still manage the erase.
    }
  }

  /// Switch both ends to [baud]. Returns false (and leaves the port at the old
  /// rate) when the bridge or chip refuses the change. Throws if the link could
  /// not be recovered after a half-applied change.
  Future<bool> changeBaud(int baud) async {
    if (baud <= 115200) return true;
    if (baud == _baud) return true; // already talking at that rate
    try {
      await _command(opChangeBaud,
          data: [..._u32(baud), ..._u32(0)],
          timeout: const Duration(seconds: 3));
    } on EspException {
      return false; // ROM kept the old rate.
    }
    try {
      await _transport.setBaud(baud);
      _baud = baud;
      await Future<void>.delayed(const Duration(milliseconds: 80));
      await flushInput();
      await readReg(0x500CA08C); // UART_DATE on the P4; any reply proves sync
      return true;
    } catch (_) {
      // The ROM may already be talking at the new rate while the host isn't
      // (or vice versa). Put both back at 115200 and re-enter the loader.
      try {
        await _transport.setBaud(115200);
        _baud = 115200;
      } catch (_) {}
      await Future<void>.delayed(const Duration(milliseconds: 100));
      try {
        if (_nativeUsb) {
          await usbReset();
        } else {
          await classicReset();
        }
        await Future<void>.delayed(const Duration(milliseconds: 250));
        if (await trySync()) {
          // A reset forgets the P4's post-connect setup.
          final info = _info;
          if (info?.chip?.id == EspChip.esp32p4.id) {
            try {
              if (info!.usbJtagSerial) await _disableWatchdogs();
              if (info.revision >= 300) await _powerOnFlash();
            } catch (_) {}
          }
          return false;
        }
      } catch (_) {}
      throw EspException('Baud rate change left the link unusable');
    }
  }

  // ------------------------------------------------------------------- flash

  /// Erase + write [image] at [offset]. [onProgress] gets 0..1.
  Future<void> writeFlash(
    Uint8List image,
    int offset, {
    required bool encryptedFlash,
    required void Function(double progress) onProgress,
    required void Function() onErasing,
    void Function()? onVerifying,
    bool Function()? isCancelled,
  }) async {
    if (offset % 0x1000 != 0) {
      throw EspException('Flash offset must be 4 KB aligned');
    }
    final padded = _pad4(image);
    final blocks = (padded.length + flashBlockSize - 1) ~/ flashBlockSize;

    final sizeMb = padded.length / (1024 * 1024);
    final eraseTimeout = Duration(
        seconds: 20 + (sizeMb * 30).ceil());

    final begin = BytesBuilder(copy: false)
      ..add(_u32(padded.length))
      ..add(_u32(blocks))
      ..add(_u32(flashBlockSize))
      ..add(_u32(offset));
    if (encryptedFlash) begin.add(_u32(0)); // begin_rom_encrypted = false
    onErasing();
    await _check(opFlashBegin,
        data: begin.takeBytes(), timeout: eraseTimeout);

    for (var seq = 0; seq < blocks; seq++) {
      if (isCancelled?.call() ?? false) {
        throw EspException('Cancelled');
      }
      final start = seq * flashBlockSize;
      final chunk = padded.sublist(
          start, start + flashBlockSize > padded.length ? padded.length : start + flashBlockSize);
      final block = Uint8List(flashBlockSize)
        ..setRange(0, chunk.length, chunk)
        ..fillRange(chunk.length, flashBlockSize, 0xFF);
      final payloadBytes = (BytesBuilder(copy: false)
            ..add(_u32(block.length))
            ..add(_u32(seq))
            ..add(_u32(0))
            ..add(_u32(0))
            ..add(block))
          .takeBytes();
      // Same retry policy as esptool: re-sending the block is safe, and one
      // lost USB transfer should not throw away an already-erased region.
      for (var attempt = 0; ; attempt++) {
        try {
          await _check(opFlashData,
              data: payloadBytes,
              chk: checksum(block),
              timeout: const Duration(seconds: 15));
          break;
        } on EspCommandException {
          // The chip actively rejected it; retrying would fail the same way.
          rethrow;
        } on EspException {
          if (attempt >= 1) rethrow;
          await flushInput();
          await Future<void>.delayed(const Duration(milliseconds: 60));
        }
      }
      onProgress((seq + 1) / blocks);
    }

    // ROM-side MD5 of what actually landed in flash.
    onVerifying?.call();
    final expected = md5.convert(padded).toString().toUpperCase();
    final md5Reply = await _check(
      opFlashMd5,
      data: [
        ..._u32(offset),
        ..._u32(padded.length),
        ..._u32(0),
        ..._u32(0),
      ],
      expectLen: 32,
      timeout: Duration(seconds: 10 + (sizeMb * 8).ceil()),
    );
    final actual = String.fromCharCodes(md5Reply).toUpperCase();
    if (actual != expected) {
      throw EspException('Flash verification failed ($actual != $expected)');
    }
  }

  Future<void> reboot() async {
    try {
      await _command(opFlashEnd,
          data: _u32(0), timeout: const Duration(seconds: 5));
    } catch (_) {
      // The chip may reboot before its ACK reaches us (or the USB port can
      // disappear first); either way the reset has been requested.
    }
    // FLASH_END only *asks* the ROM to reboot. On the native USB-Serial/JTAG
    // that request is regularly not taken and the board stays in the loader,
    // so give it a real EN pulse as well. Harmless when the chip already
    // rebooted: the port is gone and every line here is best-effort.
    await hardReset();
  }

  static Uint8List _pad4(Uint8List data) {
    final rem = data.length % 4;
    if (rem == 0) return data;
    return Uint8List(data.length + (4 - rem))
      ..setRange(0, data.length, data)
      ..fillRange(data.length, data.length + (4 - rem), 0xFF);
  }
}
