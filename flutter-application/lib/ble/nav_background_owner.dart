import 'dart:async';
import 'nav_stream.dart';

abstract interface class NavBackgroundControl {
  void setUiActive(bool active);
  void setStreamEnabled(bool enabled);
  Future<void> dispose();
}

/// Owns one background producer per BLE navigation channel. UI preferences
/// belong to the service lifetime, including when no channel exists yet.
class NavBackgroundOwner<T extends NavBackgroundControl> {
  NavBackgroundOwner(this._create);
  final T Function(NavStream) _create;
  NavStream? _channel;
  T? current;
  bool _uiActive = false;
  bool _streamEnabled = true;

  /// False for a repeated status notification about the same connection.
  bool bind(NavStream? channel) {
    if (identical(channel, _channel)) return false;
    final old = current;
    current = null;
    _channel = channel;
    if (old != null) unawaited(old.dispose());
    if (channel != null) {
      final next = _create(channel);
      next.setUiActive(_uiActive);
      next.setStreamEnabled(_streamEnabled);
      current = next;
    }
    return true;
  }

  void setUiActive(bool active) {
    _uiActive = active;
    current?.setUiActive(active);
  }

  void setStreamEnabled(bool enabled) {
    _streamEnabled = enabled;
    current?.setStreamEnabled(enabled);
  }

  Future<void> dispose() async {
    final old = current;
    current = null;
    _channel = null;
    await old?.dispose();
  }
}
