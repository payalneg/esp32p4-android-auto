/// The slice of the VESC the agent is allowed to touch.
///
/// Narrow on purpose: this interface IS the safety boundary. There is no raw
/// packet send, no `conf-set`, no current/duty command — the agent can only
/// read the script, read runtime stats and console output, write a script the
/// user approved, and stop one. A tool that doesn't exist can't be called.
///
/// It also makes the whole loop testable without hardware (see
/// test/fake_lisp_device.dart), which matters because the real path costs ~8 s
/// per read and ~15 s per flash.
library;

import '../ble/ble_proxy.dart';
import '../ble/lisp_models.dart';

abstract class LispDevice {
  bool get connected;

  /// `head-unit` or `direct:<name>` — the agent needs to know, because the
  /// console channel may not exist on a direct adapter.
  String get linkLabel;
  int get mtu;

  Future<String> readCode({void Function(double)? onProgress});

  /// Erase + write. [stopFirst] halts a running script before the erase.
  Future<void> upload(String code,
      {bool run = false,
      bool stopFirst = true,
      void Function(double)? onProgress});

  Future<void> setRunning(bool run);

  Future<LispStats> statsOnce();

  Future<LispConsoleChunk> readConsole({int sinceSeq = 0, int maxLines = 200});

  Future<void> clearConsole();
}

/// Live implementation over the background-isolate BLE stack.
class LispProxyDevice implements LispDevice {
  LispProxyDevice([LispProxy? proxy]) : _p = proxy ?? LispProxy.instance;
  final LispProxy _p;

  @override
  bool get connected => _p.target.connected;

  @override
  String get linkLabel => _p.target.kind == VescTargetKind.headUnit
      ? 'head-unit'
      : 'direct:${_p.target.name ?? '?'}';

  @override
  int get mtu => _p.target.mtu;

  @override
  Future<String> readCode({void Function(double)? onProgress}) async =>
      (await _p.read(onProgress: onProgress)).code;

  @override
  Future<void> upload(String code,
          {bool run = false,
          bool stopFirst = true,
          void Function(double)? onProgress}) =>
      _p.upload(code, run: run, stopFirst: stopFirst, onProgress: onProgress);

  @override
  Future<void> setRunning(bool run) => run ? _p.run() : _p.stop();

  @override
  Future<LispStats> statsOnce() => _p.statsOnce();

  @override
  Future<LispConsoleChunk> readConsole(
          {int sinceSeq = 0, int maxLines = 200}) =>
      _p.readConsole(sinceSeq: sinceSeq, maxLines: maxLines);

  @override
  Future<void> clearConsole() => _p.clearConsole();
}
