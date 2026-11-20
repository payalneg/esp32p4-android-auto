import 'dart:async';
import 'dart:typed_data';

import 'package:aa_bridge/ble/nav_write_queue.dart';
import 'package:flutter_test/flutter_test.dart';

void main() {
  test(
    'newest position overtakes queued map data without overlapping writes',
    () async {
      final gate = Completer<void>();
      final started = Completer<void>();
      final events = <String>[];
      var active = 0;
      final queue = NavWriteQueue(
        writeCtrl: (value) async {
          expect(active++, 0);
          events.add('view${value[0]}');
          active--;
        },
        writeData: (value, {required withoutResponse}) async {
          expect(active++, 0);
          events.add('data${value[0]}');
          if (value[0] == 0) {
            started.complete();
            await gate.future;
          }
          active--;
        },
      );
      final first = queue.writeData(
        Uint8List.fromList([0]),
        withoutResponse: true,
      );
      await started.future;
      final second = queue.writeData(
        Uint8List.fromList([1]),
        withoutResponse: true,
      );
      final oldView = queue.latestView(Uint8List.fromList([2]));
      final newView = queue.latestView(Uint8List.fromList([3]));
      gate.complete();
      await Future.wait([first, second, oldView, newView]);
      expect(events, ['data0', 'view3', 'data1']);
      queue.close();
    },
  );

  test(
    'bulk writes use acknowledged checkpoints to bound controller backlog',
    () async {
      final modes = <bool>[];
      final queue = NavWriteQueue(
        writeCtrl: (_) async {},
        writeData:
            (_, {required withoutResponse}) async => modes.add(withoutResponse),
      );
      for (var i = 0; i < 12; i++) {
        await queue.writeData(Uint8List(509), withoutResponse: true);
      }
      expect(modes, [
        true,
        true,
        true,
        true,
        false,
        true,
        true,
        true,
        true,
        false,
        true,
        true,
      ]);
      queue.close();
    },
  );

  test(
    'a failed write does not prevent the next position from being delivered',
    () async {
      var fail = true;
      final sent = <int>[];
      final queue = NavWriteQueue(
        writeCtrl: (value) async => sent.add(value[0]),
        writeData: (_, {required withoutResponse}) async {
          if (fail) {
            fail = false;
            throw StateError('BUSY');
          }
        },
      );
      await expectLater(
        queue.writeData(Uint8List(1), withoutResponse: true),
        throwsStateError,
      );
      await queue.latestView(Uint8List.fromList([7]));
      await queue.writeData(Uint8List(1), withoutResponse: true);
      expect(sent, [7]);
      queue.close();
    },
  );
}
