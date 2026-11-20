import 'dart:convert';
import 'dart:typed_data';

/// Optional ...0020 GATT protocol, independent of the UI and BLE plugin.
class RecoveryStatus {
  final bool recovery;
  final bool safe;
  final int failures;
  final int reason;
  final String slot;
  final String other;
  final int nonce;
  final int error;

  RecoveryStatus(Map<String, dynamic> json)
    : recovery = json['recovery'] as bool,
      safe = json['safe'] as bool,
      failures = json['failures'] as int,
      reason = json['reason'] as int,
      slot = json['slot'] as String,
      other = json['other'] as String,
        nonce = json['nonce'] as int,
        error = json['error'] as int? ?? 0 {
    if (json['v'] != 1 || nonce < 0 || nonce > 0xffffffff || failures < 0) {
      throw const FormatException('Unsupported recovery status');
    }
  }

  static RecoveryStatus parse(List<int> bytes) =>
      RecoveryStatus(jsonDecode(utf8.decode(bytes)) as Map<String, dynamic>);

  String get reasonName => switch (reason) {
        1 => 'Power on',
        3 => 'Software restart',
        4 => 'Panic',
        5 => 'Interrupt watchdog',
        6 => 'Task watchdog',
        7 => 'Watchdog',
        9 => 'Brownout',
        15 => 'CPU lockup',
        _ => 'Reset $reason',
      };

  List<int> command(int opcode) {
    if (opcode < 1 || opcode > 5 || (!recovery && opcode != 1)) {
      throw StateError('Command requires recovery mode');
    }
    final packet =
        ByteData(6)
          ..setUint8(0, 1)
          ..setUint8(1, opcode)
          ..setUint32(2, nonce, Endian.little);
    return packet.buffer.asUint8List();
  }
}
