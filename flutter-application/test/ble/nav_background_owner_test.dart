import 'package:aa_bridge/ble/nav_background_owner.dart';
import 'package:aa_bridge/ble/nav_stream.dart';
import 'package:flutter_test/flutter_test.dart';

class _Channel implements NavStream {
  @override
  dynamic noSuchMethod(Invocation invocation) => super.noSuchMethod(invocation);
}

class _Feed implements NavBackgroundControl {
  bool visible = false, uiActive = false, enabled = true, disposed = false;
  bool get publishing => visible && !uiActive && enabled && !disposed;
  @override
  void setUiActive(bool active) => uiActive = active;
  @override
  void setStreamEnabled(bool value) => enabled = value;
  @override
  Future<void> dispose() async {
    disposed = true;
  }
}

void main() {
  test('UI claim before connecting prevents a second GPS publisher', () async {
    final feeds = <_Feed>[];
    final owner = NavBackgroundOwner<_Feed>((_) {
      final feed = _Feed();
      feeds.add(feed);
      return feed;
    });
    owner.setUiActive(true); // IPC arrives before the BLE handshake
    final channel = _Channel();
    expect(owner.bind(channel), isTrue);
    owner.current!.visible = true;
    expect(feeds.where((f) => f.publishing), isEmpty);
    for (var i = 0; i < 5; i++) {
      expect(owner.bind(channel), isFalse); // repeated connected status
    }
    owner.setUiActive(false); // phone goes into the pocket
    expect(feeds.where((f) => f.publishing), hasLength(1));
    owner.setUiActive(true);
    expect(feeds.where((f) => f.publishing), isEmpty);
    await owner.dispose();
  });

  test(
    'reconnect retires the previous producer, including duplicate status',
    () async {
      final feeds = <_Feed>[];
      final owner = NavBackgroundOwner<_Feed>((_) {
        final feed = _Feed();
        feeds.add(feed);
        return feed;
      });
      final first = _Channel();
      owner.bind(first);
      owner.current!.visible = true;
      owner.bind(first);
      expect(feeds, hasLength(1));
      owner.bind(_Channel());
      owner.current!.visible = true;
      expect(feeds.first.disposed, isTrue);
      expect(feeds.where((f) => f.publishing), hasLength(1));
      owner.bind(null);
      expect(feeds.where((f) => f.publishing), isEmpty);
      expect(feeds.every((f) => f.disposed), isTrue);
      await owner.dispose();
    },
  );

  test(
    'stream disabled while disconnected remains disabled after reconnect',
    () async {
      final owner = NavBackgroundOwner<_Feed>((_) => _Feed());
      owner.setStreamEnabled(false);
      owner.bind(_Channel());
      owner.current!.visible = true;
      expect(owner.current!.publishing, isFalse);
      owner.bind(null);
      owner.setUiActive(true);
      owner.bind(_Channel());
      owner.current!.visible = true;
      owner.setStreamEnabled(true);
      expect(
        owner.current!.publishing,
        isFalse,
        reason: 'UI still owns navigation',
      );
      owner.setUiActive(false);
      expect(owner.current!.publishing, isTrue);
      await owner.dispose();
    },
  );
}
