/// USB-host serial link for the flasher: device discovery plus a small
/// [EspTransport] wrapper around `package:usb_serial`.
///
/// Android only: `UsbSerial` needs the USB host APIs. The plugin pops the
/// system "allow this app to access the USB device" dialog from `create()`.
library;

import 'dart:async';
import 'dart:collection';
import 'dart:typed_data';

import 'package:usb_serial/usb_serial.dart';

import 'esptool/esp_loader.dart';

/// A connected USB serial device we know how to talk to.
class UsbCandidate {
  UsbCandidate(this.device);

  final UsbDevice device;

  int get vid => device.vid ?? -1;
  int get pid => device.pid ?? -1;

  /// Espressif's own USB-Serial/JTAG (and USB-OTG) console. These boards need
  /// the USB reset sequence and must not be asked to change baud.
  bool get nativeUsb => vid == 0x303A;

  /// Bridge chip family, for the label and for deciding whether the baud rate
  /// can be raised.
  String get bridge {
    if (nativeUsb) return 'USB-JTAG';
    switch (vid) {
      case 0x1A86:
        return pid == 0x55D3 ? 'CH343' : 'CH34x';
      case 0x10C4:
        return 'CP210x';
      case 0x0403:
        return 'FTDI';
      case 0x067B:
        return 'PL2303';
      default:
        return 'USB UART';
    }
  }

  String get label {
    final name = device.productName?.trim();
    final maker = device.manufacturerName?.trim();
    final desc = (name != null && name.isNotEmpty)
        ? name
        : ((maker != null && maker.isNotEmpty) ? maker : bridge);
    return '$desc · $bridge';
  }

  String get id => '${vid.toRadixString(16).padLeft(4, '0')}:'
      '${pid.toRadixString(16).padLeft(4, '0')}';

  @override
  bool operator ==(Object other) =>
      other is UsbCandidate && other.device.deviceId == device.deviceId;

  @override
  int get hashCode => device.deviceId.hashCode;
}

/// Devices we can drive. Listing every attached USB device would also show
/// keyboards and hubs, which is not what the user is looking for.
bool _canDrive(UsbDevice d) {
  final vid = d.vid;
  final pid = d.pid;
  if (vid == null || pid == null) return false;
  switch (vid) {
    case 0x303A: // Espressif native USB (CDC-ACM)
      return true;
    case 0x1A86: // CH340 / CH341 / CH343
      return pid == 0x7523 || pid == 0x5523 || pid == 0x55D3;
    case 0x10C4: // CP210x
      return pid == 0xEA60 || pid == 0xEA70 || pid == 0xEA71;
    case 0x0403: // FTDI
      return pid == 0x6001 || pid == 0x6010 || pid == 0x6011 ||
          pid == 0x6014 || pid == 0x6015;
    case 0x067B: // Prolific
      return true;
    default:
      return false;
  }
}

class UsbDevices {
  /// Attached devices that look like ESP USB-serial ports, one entry per
  /// physical device (the first serial port of it).
  static Future<List<UsbCandidate>> scan() async {
    final all = await UsbSerial.listDevices();
    final seen = <int>{};
    final out = <UsbCandidate>[];
    for (final d in all) {
      if (!_canDrive(d)) continue;
      final id = d.deviceId;
      if (id == null || !seen.add(id)) continue;
      out.add(UsbCandidate(d));
    }
    // Native USB first: it is the port that needs no bridge driver.
    out.sort((a, b) => a.nativeUsb == b.nativeUsb
        ? a.label.compareTo(b.label)
        : (a.nativeUsb ? -1 : 1));
    return out;
  }
}

class UsbLinkException extends EspException {
  UsbLinkException(super.message);
}

/// [EspTransport] implementation over a `UsbPort`.
class UsbSerialLink implements EspTransport {
  UsbSerialLink(this._candidate);

  final UsbCandidate _candidate;
  UsbPort? _port;
  StreamSubscription<Uint8List>? _rx;
  final Queue<Uint8List> _queue = Queue<Uint8List>();
  Completer<void>? _signal;
  bool _closed = false;

  bool get isOpen => _port != null && !_closed;

  Future<void> open({int baud = 115200}) async {
    final port = await _candidate.device.create();
    if (port == null) {
      throw UsbLinkException('Could not open ${_candidate.label}');
    }
    _port = port;
    // Remembered by the plugin and applied when the port opens.
    await port.setPortParameters(
      baud,
      UsbPort.DATABITS_8,
      UsbPort.STOPBITS_1,
      UsbPort.PARITY_NONE,
    );
    try {
      await port.setFlowControl(UsbPort.FLOW_CONTROL_OFF);
    } catch (_) {}
    try {
      await port.setDTR(false);
      await port.setRTS(false);
    } catch (_) {}
    final ok = await port.open();
    if (!ok) {
      throw UsbLinkException('Could not open ${_candidate.label}');
    }
    _rx = port.inputStream?.listen(
      _onData,
      onError: (Object e) {
        _signal?.complete();
        _signal = null;
      },
      onDone: () {
        _signal?.complete();
        _signal = null;
      },
    );
  }

  void _onData(Uint8List chunk) {
    if (chunk.isEmpty) return;
    _queue.add(chunk);
    final signal = _signal;
    _signal = null;
    if (signal != null && !signal.isCompleted) signal.complete();
  }

  @override
  Future<Uint8List?> readChunk(Duration timeout) async {
    if (_queue.isNotEmpty) return _queue.removeFirst();
    final signal = _signal ??= Completer<void>();
    try {
      await signal.future.timeout(timeout);
    } on TimeoutException {
      return null;
    }
    return _queue.isEmpty ? null : _queue.removeFirst();
  }

  @override
  Future<void> write(Uint8List data) async {
    final port = _port;
    if (port == null) throw UsbLinkException('USB port is closed');
    await port.write(data);
  }

  @override
  Future<void> flushInput() async {
    _queue.clear();
    // Give the adapter a moment to hand over bytes already in flight, so a
    // stale reply cannot be matched to the next command.
    final deadline = DateTime.now().add(const Duration(milliseconds: 60));
    while (DateTime.now().isBefore(deadline)) {
      final chunk = await readChunk(const Duration(milliseconds: 15));
      if (chunk == null) break;
    }
    _queue.clear();
  }

  @override
  Future<void> setBaud(int baud) async {
    final port = _port;
    if (port == null) throw UsbLinkException('USB port is closed');
    await port.setPortParameters(
      baud,
      UsbPort.DATABITS_8,
      UsbPort.STOPBITS_1,
      UsbPort.PARITY_NONE,
    );
  }

  @override
  Future<void> setDtr(bool value) async {
    await _port?.setDTR(value);
  }

  @override
  Future<void> setRts(bool value) async {
    await _port?.setRTS(value);
  }

  @override
  Future<void> close() async {
    _closed = true;
    final sub = _rx;
    _rx = null;
    await sub?.cancel();
    final port = _port;
    _port = null;
    if (port != null) {
      try {
        await port.close();
      } catch (_) {}
    }
    final signal = _signal;
    _signal = null;
    if (signal != null && !signal.isCompleted) signal.complete();
  }
}
