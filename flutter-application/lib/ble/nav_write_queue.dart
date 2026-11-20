import 'dart:async';
import 'dart:typed_data';

/// Serialize navigation writes, placing the newest position before bulk work.
/// Acknowledged checkpoints bound the amount of map data queued in Android's
/// controller, where application-level priority can no longer reorder it.
class NavWriteQueue {
  NavWriteQueue({
    required Future<void> Function(Uint8List) writeCtrl,
    required Future<void> Function(Uint8List, {required bool withoutResponse})
    writeData,
  }) : _ctrl = writeCtrl,
       _data = writeData;

  static const maxUnackedBytes = 2048;
  final Future<void> Function(Uint8List) _ctrl;
  final Future<void> Function(Uint8List, {required bool withoutResponse}) _data;
  Future<void> _tail = Future<void>.value();
  Uint8List? _view;
  Completer<void>? _viewDone;
  int _unackedBytes = 0;
  bool _closed = false;

  Future<void> _enqueue(Future<void> Function() operation) {
    if (_closed) {
      return Future<void>.error(StateError('Navigation link closed'));
    }
    final next = _tail.then((_) async {
      if (_closed) throw StateError('Navigation link closed');
      await _flushView();
      if (_closed) throw StateError('Navigation link closed');
      await operation();
    });
    // An individual write failure must not poison subsequent operations.
    _tail = next.catchError((Object _) {});
    return next;
  }

  Future<void> _flushView() async {
    final view = _view;
    final done = _viewDone;
    if (view == null || done == null) return;
    _view = null;
    _viewDone = null;
    try {
      await _ctrl(view);
      _unackedBytes = 0;
      done.complete();
    } catch (error, stack) {
      done.completeError(error, stack);
    }
  }

  /// Superseded callers share completion of the single newest pending VIEW.
  /// A write already in progress cannot be preempted.
  Future<void> latestView(Uint8List value) {
    if (_closed) {
      return Future<void>.error(StateError('Navigation link closed'));
    }
    _view = Uint8List.fromList(value);
    if (_viewDone != null) return _viewDone!.future;
    final done = _viewDone = Completer<void>();
    unawaited(
      _enqueue(() async {}).catchError((Object error, StackTrace stack) {
        if (identical(_viewDone, done)) {
          _view = null;
          _viewDone = null;
          done.completeError(error, stack);
        }
      }),
    );
    return done.future;
  }

  Future<void> writeCtrl(Uint8List value) => _enqueue(() async {
    await _ctrl(value);
    _unackedBytes = 0;
  });

  Future<void> writeData(Uint8List value, {required bool withoutResponse}) =>
      _enqueue(() async {
        final fast =
            withoutResponse && _unackedBytes + value.length <= maxUnackedBytes;
        await _data(value, withoutResponse: fast);
        _unackedBytes = fast ? _unackedBytes + value.length : 0;
      });

  void close() {
    _closed = true;
    final done = _viewDone;
    _view = null;
    _viewDone = null;
    done?.completeError(StateError('Navigation link closed'));
  }
}
