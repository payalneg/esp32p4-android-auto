/// Cooperative cancellation for the agent loop.
///
/// Nothing else in this app can be cancelled, so this is built here and only
/// here. Note the asymmetry it has to model:
///
///   * The network CAN be cut mid-flight — `HttpClient.close(force: true)`
///     errors the response stream immediately.
///   * A device transfer CANNOT. `VescLink` serialises whole operations
///     against the head unit's single CAN reassembly buffer, so a 23 KB
///     upload runs to completion whatever the user taps. The token is checked
///     at step boundaries and before each tool dispatch, and the UI says
///     "stopping after the current transfer" rather than pretending otherwise.
library;

import 'dart:async';

class AgentCancelled implements Exception {
  const AgentCancelled();
  @override
  String toString() => 'AgentCancelled';
}

class CancelToken {
  final _completer = Completer<void>();
  final _onCancel = <void Function()>[];

  bool get isCancelled => _completer.isCompleted;

  /// Resolves when cancelled — useful with `Future.any` to race a wait.
  Future<void> get whenCancelled => _completer.future;

  /// Register a hook run at cancel time (e.g. force-closing an HttpClient).
  /// Runs immediately if the token is already cancelled.
  void onCancel(void Function() fn) {
    if (isCancelled) {
      fn();
      return;
    }
    _onCancel.add(fn);
  }

  void removeHook(void Function() fn) => _onCancel.remove(fn);

  void cancel() {
    if (isCancelled) return;
    _completer.complete();
    for (final fn in List.of(_onCancel)) {
      try {
        fn();
      } catch (_) {
        // A failing hook must not stop the others.
      }
    }
    _onCancel.clear();
  }

  void throwIfCancelled() {
    if (isCancelled) throw const AgentCancelled();
  }
}
