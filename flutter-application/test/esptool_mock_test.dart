import 'dart:collection';
import 'dart:typed_data';

import 'package:aa_bridge/firmware/esptool/esp_chip.dart';
import 'package:aa_bridge/firmware/esptool/esp_loader.dart';
import 'package:aa_bridge/firmware/esptool/slip.dart';
import 'package:crypto/crypto.dart' show md5;
import 'package:flutter_test/flutter_test.dart';

/// A software ESP32-P4 ROM loader: keeps a flash image and speaks the serial
/// protocol well enough to exercise EspLoader without hardware.
class FakeRom implements EspTransport {
  FakeRom({int revision = 100}) {
    final major = revision ~/ 100;
    final minor = revision % 100;
    _regs[0x5012D04C] = (major << 4) | minor; // EFUSE_BLOCK1 word 2
    _regs[0x4FF3FEC8] = 0; // UARTDEV_BUF_NO -> UART, not USB
    _regs[0x500CA08C] = 0x12345678; // UART_DATE
  }

  final Uint8List _flash = Uint8List(0x400000);
  final Map<int, int> _regs = <int, int>{};
  final Map<int, int> writes = <int, int>{};
  final SlipDecoder _decoder = SlipDecoder();
  final Queue<Uint8List> _out = Queue<Uint8List>();

  int flashOffset = 0;
  int flashSize = 0;
  int flashBlocks = 0;
  int flashBlockSize = 0;
  int beginParams = 0;
  int blocksWritten = 0;
  bool began = false;

  int baud = 115200;
  bool closed = false;

  @override
  Future<void> write(Uint8List data) async {
    for (final frame in _decoder.add(data)) {
      _handle(frame);
    }
  }

  @override
  Future<Uint8List?> readChunk(Duration timeout) async {
    if (_out.isNotEmpty) return _out.removeFirst();
    return null;
  }

  @override
  Future<void> flushInput() async => _out.clear();

  @override
  Future<void> setBaud(int value) async => baud = value;

  @override
  Future<void> setDtr(bool value) async {}

  @override
  Future<void> setRts(bool value) async {}

  @override
  Future<void> close() async => closed = true;

  void _reply(int op, {int value = 0, Uint8List? payload}) {
    final data = payload ?? Uint8List.fromList(<int>[0, 0]);
    final header = ByteData(8)
      ..setUint8(0, 0x01)
      ..setUint8(1, op)
      ..setUint16(2, data.length, Endian.little)
      ..setUint32(4, value, Endian.little);
    final packet = BytesBuilder(copy: false)
      ..add(header.buffer.asUint8List())
      ..add(data);
    _out.add(slipEncode(packet.takeBytes()));
  }

  void _status(int op, int status, [int reason = 0]) =>
      _reply(op, payload: Uint8List.fromList(<int>[status, reason]));

  static int _u32(Uint8List d, int at) =>
      ByteData.sublistView(d, at, at + 4).getUint32(0, Endian.little);

  void _handle(Uint8List frame) {
    if (frame.length < 8 || frame[0] != 0x00) return;
    final op = frame[1];
    final size = ByteData.sublistView(frame, 0, 8).getUint16(2, Endian.little);
    final chk = _u32(frame, 4);
    final data = frame.sublist(8, 8 + size);

    switch (op) {
      case 0x08: // SYNC: the ROM answers eight times
        for (var i = 0; i < 8; i++) {
          _reply(op, value: 0x20120707);
        }
        break;
      case 0x14: // GET_SECURITY_INFO
        final info = Uint8List(20);
        ByteData.sublistView(info).setUint32(12, 18, Endian.little);
        _reply(op, payload: Uint8List.fromList([...info, 0, 0]));
        break;
      case 0x0A: // READ_REG
        final addr = _u32(data, 0);
        if (addr == 0xBAD) {
          _status(op, 0x05, 0x05); // unsupported/invalid, like a ROM error
          break;
        }
        _reply(op, value: _regs[addr] ?? 0);
        break;
      case 0x09: // WRITE_REG
        final addr = _u32(data, 0);
        _regs[addr] = _u32(data, 4);
        writes[addr] = _u32(data, 4);
        _status(op, 0);
        break;
      case 0x0D: // SPI_ATTACH
        _status(op, 0);
        break;
      case 0x0F: // CHANGE_BAUDRATE
        _status(op, 0);
        break;
      case 0x02: // FLASH_BEGIN
        began = true;
        flashSize = _u32(data, 0);
        flashBlocks = _u32(data, 4);
        flashBlockSize = _u32(data, 8);
        flashOffset = _u32(data, 12);
        beginParams = data.length;
        blocksWritten = 0;
        _status(op, 0);
        break;
      case 0x03: // FLASH_DATA
        final len = _u32(data, 0);
        final seq = _u32(data, 4);
        final block = data.sublist(16, 16 + len);
        final expected = EspLoader.checksum(block);
        if (chk != expected) {
          _status(op, 1, 0xC1);
          break;
        }
        final at = flashOffset + seq * flashBlockSize;
        _flash.setRange(at, at + len, block);
        blocksWritten++;
        _status(op, 0);
        break;
      case 0x13: // SPI_FLASH_MD5
        final at = _u32(data, 0);
        final len = _u32(data, 4);
        final digest = md5
            .convert(Uint8List.sublistView(_flash, at, at + len))
            .toString()
            .toUpperCase();
        _reply(op,
            payload: Uint8List.fromList(
                [...digest.codeUnits, 0, 0]));
        break;
      case 0x04: // FLASH_END
        _status(op, 0);
        break;
      default:
        _status(op, 0x05, 0x05); // unsupported command
    }
  }

  Uint8List flashAt(int offset, int length) =>
      Uint8List.fromList(Uint8List.sublistView(_flash, offset, offset + length));
}

void main() {
  test('connects to the fake P4 ROM loader', () async {
    final rom = FakeRom();
    final loader = EspLoader(rom);
    final device = await loader.connect(nativeUsb: false);
    expect(device.chipId, 18);
    expect(device.chip, EspChip.esp32p4);
    expect(device.revision, 100);
    expect(device.usbJtagSerial, isFalse);
  });

  test('P4 3.x runs the flash power-on sequence, 1.x does not', () async {
    final v3 = FakeRom(revision: 302);
    await EspLoader(v3).connect(nativeUsb: false);
    expect(v3.writes[0x5011010C], 1,
        reason: 'PMU pad group must be powered before SPI_ATTACH');

    final v1 = FakeRom(revision: 100);
    await EspLoader(v1).connect(nativeUsb: false);
    expect(v1.writes.containsKey(0x5011010C), isFalse);
  });

  test('writes an image and verifies the ROM MD5', () async {
    final rom = FakeRom();
    final loader = EspLoader(rom);
    await loader.connect(nativeUsb: false);
    await loader.attachFlash();

    final image =
        Uint8List.fromList(List<int>.generate(40000, (i) => (i * 37) & 0xFF));
    final progress = <double>[];
    var erasing = false;
    await loader.writeFlash(
      image,
      0x20000,
      encryptedFlash: EspChip.esp32p4.encryptedFlash,
      onErasing: () => erasing = true,
      onProgress: progress.add,
      onVerifying: () {},
    );

    expect(erasing, isTrue);
    expect(rom.beginParams, 20, reason: 'P4 FLASH_BEGIN has the extra flag');
    expect(rom.flashOffset, 0x20000);
    expect(rom.blocksWritten, rom.flashBlocks);
    expect(progress.first, closeTo(1 / rom.flashBlocks, 1e-9));
    expect(progress.last, 1.0);
    // Padded to 4 bytes and written verbatim, including the 0xFF tail.
    final expected = Uint8List(40000)..setAll(0, image);
    expect(rom.flashAt(0x20000, 40000), expected);
  });

  test('rejects an offset that the ROM cannot write unaligned', () async {
    final rom = FakeRom();
    final loader = EspLoader(rom);
    await loader.connect(nativeUsb: false);
    await expectLater(
      loader.writeFlash(
        Uint8List.fromList(<int>[1, 2, 3, 4]),
        0x1234,
        encryptedFlash: true,
        onErasing: () {},
        onProgress: (_) {},
      ),
      throwsA(isA<EspException>()),
    );
  });

  test('changes baud and pings the chip (UART bridge path)', () async {
    final rom = FakeRom();
    final loader = EspLoader(rom);
    await loader.connect(nativeUsb: false);
    expect(await loader.changeBaud(921600), isTrue);
    expect(rom.baud, 921600);
  });

  test('a ROM that refuses a command surfaces a command exception', () async {
    final rom = FakeRom();
    final loader = EspLoader(rom);
    await loader.connect(nativeUsb: false);
    // The fake answers status 0x05 (invalid command) for this address.
    await expectLater(
      loader.readReg(0xBAD),
      throwsA(isA<EspCommandException>()),
    );
  });
}
