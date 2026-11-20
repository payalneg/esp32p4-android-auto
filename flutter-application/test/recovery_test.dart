import 'dart:convert';
import 'package:flutter_test/flutter_test.dart';
import 'package:aa_bridge/firmware/recovery.dart';

Map<String, dynamic> status({bool recovery = true}) => {
  'v': 1,
  'recovery': recovery,
  'safe': false,
  'failures': 3,
  'reason': 6,
  'slot': 'ota_1',
  'other': '1.3.49',
  'nonce': 0xfedcba98,
};

void main() {
  test('firmware status and unsigned nonce round trip', () {
    final s = RecoveryStatus.parse(utf8.encode(jsonEncode(status())));
    expect(s.recovery, isTrue);
    expect(s.other, '1.3.49');
    expect(s.command(3), [1, 3, 0x98, 0xba, 0xdc, 0xfe]);
  });
  test('destructive commands require recovery', () {
    final s = RecoveryStatus(status(recovery: false));
    expect(s.command(1)[1], 1);
    for (final op in [0, 2, 3, 4, 5, 6]) {
      expect(() => s.command(op), throwsStateError);
    }
  });
  test('unsupported version and invalid nonce rejected', () {
    expect(() => RecoveryStatus({...status(), 'v': 2}), throwsFormatException);
    expect(
      () => RecoveryStatus({...status(), 'nonce': -1}),
      throwsFormatException,
    );
    expect(
      () => RecoveryStatus({...status(), 'nonce': 0x100000000}),
      throwsFormatException,
    );
    expect(() => RecoveryStatus.parse([123]), throwsFormatException);
  });
}
