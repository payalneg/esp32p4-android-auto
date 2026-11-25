/// Wire format for the "new firmware" dialog the phone raises on the head
/// unit, and the version comparison that decides whether to raise it at all.
library;

import 'package:aa_bridge/ble/fw_prompt.dart';
import 'package:aa_bridge/firmware/firmware_updater.dart';
import 'package:flutter_test/flutter_test.dart';

void main() {
  group('FwPromptOffer', () {
    FwPromptOffer offer({String title = 'Firmware update available'}) =>
        FwPromptOffer(
          current: '1.3.52',
          available: '1.3.55',
          title: title,
          body: 'Update the head unit now?',
          now: 'Update',
          later: 'Later',
          skip: 'Skip this version',
          updating: 'Updating',
        );

    test('encodes an OFFER the firmware can split line by line', () {
      final lines = offer().encode().split('\n');
      expect(lines, hasLength(9));
      expect(lines[0], 'OFFER');
      expect(lines[1], '1.3.52');
      expect(lines[2], '1.3.55');
      expect(lines[3], 'Firmware update available');
      expect(lines[8], 'Updating');
    });

    test('newlines in a translated label stay one field', () {
      final lines = offer(title: 'Firmware\nupdate').encode().split('\n');
      expect(lines, hasLength(9));
      expect(lines[3], 'Firmware update');
    });

    test('progress and failure stay single lines', () {
      expect(FwPromptOffer.progress(42), 'PROGRESS\n42');
      expect(FwPromptOffer.progress(140), 'PROGRESS\n100');
      expect(FwPromptOffer.progress(-5), 'PROGRESS\n0');
      expect(FwPromptOffer.failed('nope\nreally'), 'FAILED\nnope really');
    });
  });

  group('parseFwPromptChoice', () {
    test('accepts the one-byte and text answers', () {
      expect(parseFwPromptChoice([1]), FwPromptChoice.update);
      expect(parseFwPromptChoice('NOW'.codeUnits), FwPromptChoice.update);
      expect(parseFwPromptChoice([0]), FwPromptChoice.later);
      expect(parseFwPromptChoice([2]), FwPromptChoice.skip);
      expect(parseFwPromptChoice('LATER\n'.codeUnits), FwPromptChoice.later);
      expect(parseFwPromptChoice('N'.codeUnits), FwPromptChoice.later);
      expect(parseFwPromptChoice('SKIP'.codeUnits), FwPromptChoice.skip);
      expect(parseFwPromptChoice('S'.codeUnits), FwPromptChoice.skip);
    });

    test('garbage is ignored rather than read as an update', () {
      expect(parseFwPromptChoice(const []), isNull);
      expect(parseFwPromptChoice('what'.codeUnits), isNull);
      expect(parseFwPromptChoice([0x42, 0xFF]), isNull);
    });
  });

  group('FirmwareUpdater.compareVersions', () {
    test('orders dotted releases numerically', () {
      expect(FirmwareUpdater.compareVersions('1.3.55', '1.3.54'), 1);
      expect(FirmwareUpdater.compareVersions('1.3.9', '1.3.10'), -1);
      expect(FirmwareUpdater.compareVersions('1.3.55', '1.3.55'), 0);
      expect(FirmwareUpdater.compareVersions('1.2', '1.2.0'), 0);
    });

    test('a suffix does not invent a new version', () {
      expect(FirmwareUpdater.compareVersions('1.3.55-rc1', '1.3.55'), 0);
    });
  });

  test('skipped versions are remembered per board', () {
    expect(FirmwareUpdater.skippedVersionKey('waveshare'),
        isNot(FirmwareUpdater.skippedVersionKey('jc4880')));
  });
}
