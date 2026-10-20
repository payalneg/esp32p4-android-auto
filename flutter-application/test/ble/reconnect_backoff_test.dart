import 'package:aa_bridge/ble/reconnect_backoff.dart';
import 'package:flutter_test/flutter_test.dart';

void main() {
  test('an absent head unit does not restart autoConnect every 40 seconds', () {
    final backoff = ReconnectBackoff();
    var elapsed = Duration.zero;
    var retries = 0;
    const hour = Duration(hours: 1);
    while (elapsed + backoff.delay <= hour) {
      expect(backoff.delay, lessThanOrEqualTo(const Duration(minutes: 5)));
      elapsed += backoff.delay;
      retries++;
      backoff.didRetry();
    }
    // The former fixed interval caused about 90 cancellations/restarts per
    // hour. Keep a bounded recovery path without that background churn.
    expect(retries, inInclusiveRange(12, 15));
  });

  test(
    'a successful connection restores prompt recovery for the next loss',
    () {
      final backoff = ReconnectBackoff();
      for (var retry = 0; retry < 20; retry++) {
        backoff.didRetry();
      }
      backoff.reset();
      expect(backoff.delay, const Duration(seconds: 40));
    },
  );
}
