/// High-level "flash the head unit over a USB cable" flow.
///
/// Wraps [UsbSerialLink] + [EspLoader] and turns the protocol into a stream of
/// localisable progress states for the firmware screen. Nothing here throws:
/// failures come out of [state] and the bool result.
library;

import 'dart:async';
import 'dart:typed_data';

import 'package:flutter/foundation.dart' show debugPrint;
import 'package:flutter/services.dart' show PlatformException;

import 'esptool/esp_chip.dart';
import 'esptool/esp_loader.dart';
import 'esp_image.dart';
import 'usb_serial_link.dart';

enum UsbFlashPhase {
  idle,
  opening,
  connecting,
  checking,
  erasing,
  writing,
  verifying,
  rebooting,
  done,
  error,
}

class UsbFlashState {
  const UsbFlashState(
    this.phase, {
    this.progress = 0,
    this.messageKey,
    this.args,
    this.chip,
  });

  final UsbFlashPhase phase;

  /// 0..1 while [phase] == writing.
  final double progress;

  /// i18n key, localised by the screen (no BuildContext here).
  final String? messageKey;
  final Map<String, String>? args;

  /// Chip name reported by the ROM, once known.
  final String? chip;

  bool get busy => phase != UsbFlashPhase.idle &&
      phase != UsbFlashPhase.done &&
      phase != UsbFlashPhase.error;
}

class UsbFlashException implements Exception {
  UsbFlashException(this.key, [this.args]);

  final String key;
  final Map<String, String>? args;
}

class UsbFlasher {
  final StreamController<UsbFlashState> _ctrl =
      StreamController<UsbFlashState>.broadcast();

  UsbFlashState _last = const UsbFlashState(UsbFlashPhase.idle);
  Stream<UsbFlashState> get state => _ctrl.stream;
  UsbFlashState get last => _last;

  bool _cancelled = false;
  UsbSerialLink? _link;

  /// Last phase/percent written to logcat, so the write progress does not
  /// flood it.
  UsbFlashPhase? _loggedPhase;
  int _loggedPct = -1;

  /// Progress states also go to logcat (`adb logcat -s flutter`), because the
  /// screen only ever shows one line and the interesting part of a failed USB
  /// flash — which phase, which chip, what the ROM said — is gone as soon as
  /// the error replaces it. The device side of the same story is on the Wi-Fi
  /// debug bridge (`scripts/wifi_debug.py logs`).
  void _log(UsbFlashPhase phase, String? messageKey, Map<String, String>? args,
      String? chip, int pct) {
    final phaseChanged = phase != _loggedPhase;
    final step = pct >= 0 && (pct / 10).floor() != (_loggedPct / 10).floor();
    if (!phaseChanged && !step) return;
    _loggedPhase = phase;
    _loggedPct = pct;
    final parts = <String>['[usbflash]', phase.name];
    if (chip != null) parts.add('chip=$chip');
    if (pct >= 0) parts.add('$pct%');
    if (messageKey != null) parts.add(messageKey);
    if (args != null && args.isNotEmpty) {
      parts.add(args.entries.map((e) => '${e.key}=${e.value}').join(' '));
    }
    debugPrint(parts.join(' '));
  }

  void _emit(
    UsbFlashPhase phase, {
    double progress = 0,
    String? messageKey,
    Map<String, String>? args,
    String? chip,
  }) {
    if (_ctrl.isClosed) return;
    _last = UsbFlashState(phase,
        progress: progress, messageKey: messageKey, args: args, chip: chip);
    _log(phase, messageKey, args, chip,
        phase == UsbFlashPhase.writing ? (progress * 100).round() : -1);
    _ctrl.add(_last);
  }

  /// Abort an in-flight flash. The port is closed, which makes the pending
  /// ROM command fail fast; [run] then reports the cancellation.
  void cancel() {
    _cancelled = true;
    unawaited(_link?.close());
  }

  void dispose() => _ctrl.close();

  /// Write [image] (a merged image or an application .bin) to [candidate].
  ///
  /// [appOffset] is where an application-only image goes; merged images always
  /// start at 0x0. The image header's chip id/revision are checked against the
  /// attached chip before anything is erased.
  Future<bool> run({
    required UsbCandidate candidate,
    required Uint8List image,
    required String imageName,
    int appOffset = 0x20000,
  }) async {
    _cancelled = false;
    final info = inspectEspImage(image);
    if (!info.isValid) {
      _emit(UsbFlashPhase.error, messageKey: info.errorKey);
      return false;
    }
    if (!info.isMerged && appOffset % 0x1000 != 0) {
      _emit(UsbFlashPhase.error, messageKey: 'fw.usb.err.offset');
      return false;
    }
    // 32 MB is the largest flash any supported board has; anything bigger is
    // almost certainly not a firmware image.
    if (image.length > 0x2000000) {
      _emit(UsbFlashPhase.error, messageKey: 'fw.usb.err.toobig');
      return false;
    }

    final offset = info.isMerged ? 0 : appOffset;
    final sizeKb = (image.length / 1024).round().toString();
    final link = UsbSerialLink(candidate);
    _link = link;
    try {
      _emit(UsbFlashPhase.opening,
          messageKey: 'fw.usb.phase.open', args: {'device': candidate.label});
      await link.open();
      if (_cancelled) throw UsbFlashException('fw.usb.cancelled');

      _emit(UsbFlashPhase.connecting,
          messageKey: 'fw.usb.phase.reset', args: {'device': candidate.label});
      final loader = EspLoader(link);
      final device = await loader.connect(nativeUsb: candidate.nativeUsb);
      if (_cancelled) throw UsbFlashException('fw.usb.cancelled');

      final chipLabel = device.chip?.name != null
          ? '${device.chip!.name} rev '
              '${device.revision ~/ 100}.${device.revision % 100}'
              '${device.usbJtagSerial ? " (usb-jtag)" : ""}'
          : (device.chipId != null ? 'chip ${device.chipId}' : 'unknown chip');
      _emit(UsbFlashPhase.checking,
          messageKey: 'fw.usb.phase.check', args: {'chip': chipLabel}, chip: chipLabel);

      // Refuse a chip/image mismatch before the first erase. Without a chip id
      // in the image this is a best-effort check only.
      final imageChipId = info.chipId;
      if (imageChipId != null &&
          device.chipId != null &&
          imageChipId != device.chipId) {
        final imageChip = EspChip.byId(imageChipId)?.name ?? 'chip $imageChipId';
        throw UsbFlashException('fw.usb.err.chip',
            {'image': imageChip, 'chip': device.name});
      }
      // Only the P4 has its revision read (and images compiled per revision).
      if (device.chip?.id == EspChip.esp32p4.id &&
          imageChipId == EspChip.esp32p4.id &&
          !info.acceptsRevision(device.revision)) {
        throw UsbFlashException('fw.usb.err.rev', {
          'range': info.revisionRange,
          'major': '${device.revision ~/ 100}',
          'minor': '${device.revision % 100}',
        });
      }

      if (!candidate.nativeUsb) {
        // UART bridges: raise the baud rate. 921600 (what esptool defaults to)
        // did not survive the round trip on the phone's USB-OTG + CH343: the
        // ROM switched, but the verification read timed out every time and the
        // link had to be reset back. 460800 is 4x the default and reliable.
        // A failure is not fatal — the link is simply left at 115200 and the
        // write takes longer.
        await loader.changeBaud(460800);
      }
      // Configure the flash pins at whatever rate we ended up at, take the
      // flash part out of whatever state the application left it in (0x66/0x99)
      // and tell the ROM how big the chip is — without that last step the ROM
      // thinks the flash has size 0 and the first erase fails with
      // COMMAND_FAILED.
      await loader.attachFlash();
      await loader.resetFlashChip();
      final flashId = await loader.flashId();
      debugPrint('[usbflash] flash id=0x${flashId.toRadixString(16)}');
      var flashSize = ((image.length + 0xFFFFF) ~/ 0x100000) * 0x100000;
      if (flashSize < 4 * 1024 * 1024) flashSize = 4 * 1024 * 1024;
      await loader.setFlashParams(flashSize);
      if (_cancelled) throw UsbFlashException('fw.usb.cancelled');

      await loader.writeFlash(
        image,
        offset,
        encryptedFlash: device.chip?.encryptedFlash ?? true,
        onErasing: () => _emit(UsbFlashPhase.erasing,
            messageKey: 'fw.usb.phase.erase',
            args: {'size': sizeKb},
            chip: chipLabel),
        onProgress: (p) => _emit(UsbFlashPhase.writing,
            progress: p,
            messageKey: 'fw.usb.phase.write',
            args: {'name': imageName},
            chip: chipLabel),
        onVerifying: () => _emit(UsbFlashPhase.verifying,
            messageKey: 'fw.usb.phase.verify', chip: chipLabel),
        isCancelled: () => _cancelled,
      );

      // An application-only write at 0x20000 lands in ota_0, but the otadata
      // partition may still point at ota_1 from an earlier OTA — the device
      // would reboot into the old firmware. Erase otadata (0xF000, 8 KB) so
      // the bootloader falls back to ota_0. The layout is this project's P4
      // table; a full recovery image handles foreign layouts by definition.
      if (!info.isMerged &&
          offset == 0x20000 &&
          device.chip?.id == EspChip.esp32p4.id) {
        final blank = Uint8List(0x2000)..fillRange(0, 0x2000, 0xFF);
        await loader.writeFlash(
          blank,
          0xF000,
          encryptedFlash: device.chip?.encryptedFlash ?? true,
          onErasing: () {},
          onProgress: (_) {},
          isCancelled: () => _cancelled,
        );
      }

      _emit(UsbFlashPhase.rebooting,
          messageKey: 'fw.usb.phase.reboot', chip: chipLabel);
      await loader.reboot();
      _emit(UsbFlashPhase.done, messageKey: 'fw.usb.done', chip: chipLabel);
      return true;
    } on UsbFlashException catch (e) {
      _emit(UsbFlashPhase.error, messageKey: e.key, args: e.args);
      return false;
    } on EspTimeoutException catch (e) {
      // Before the handshake finishes the fix is a manual BOOT/RESET; once the
      // chip answered, a timeout means the transfer stalled.
      final handshaking = _last.phase == UsbFlashPhase.opening ||
          _last.phase == UsbFlashPhase.connecting;
      _emit(UsbFlashPhase.error,
          messageKey: _cancelled
              ? 'fw.usb.cancelled'
              : (handshaking ? 'fw.usb.err.sync' : 'fw.usb.err.timeout'),
          args: {'err': e.message});
      return false;
    } on EspCommandException catch (e) {
      _emit(UsbFlashPhase.error,
          messageKey: 'fw.usb.err.proto', args: {'err': e.message});
      return false;
    } on PlatformException catch (e) {
      final msg = '${e.message ?? ''} ${e.details ?? ''}'.toLowerCase();
      _emit(UsbFlashPhase.error,
          messageKey: msg.contains('permission')
              ? 'fw.usb.err.permission'
              : 'fw.usb.err.open',
          args: {'err': e.message ?? e.code});
      return false;
    } on EspException catch (e) {
      _emit(UsbFlashPhase.error,
          messageKey: _cancelled ? 'fw.usb.cancelled' : 'fw.usb.err.generic',
          args: {'err': e.message});
      return false;
    } catch (e) {
      _emit(UsbFlashPhase.error,
          messageKey: _cancelled ? 'fw.usb.cancelled' : 'fw.usb.err.generic',
          args: {'err': '$e'});
      return false;
    } finally {
      try {
        await link.close();
      } catch (_) {}
      _link = null;
    }
  }
}
