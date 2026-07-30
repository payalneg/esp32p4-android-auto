/// Message keys for the UI↔background-isolate port (flutter_foreground_task).
///
/// All BLE now lives in the foreground-service task isolate (see ble_host.dart)
/// so the connection survives the UI being backgrounded or killed. The UI talks
/// to it through [FlutterForegroundTask.sendDataToTask] /
/// [FlutterForegroundTask.sendDataToMain]. That channel JSON-round-trips its
/// payload, so EVERYTHING here must be JSON-safe: ints / doubles / bools /
/// Strings / Lists / Maps only — binary travels as base64 Strings.
library;

/// UI → task: every request carries `cmd` and (when it expects a reply) `id`.
class IpcCmd {
  static const getStatus = 'getStatus';
  static const scan = 'scan';
  static const connect = 'connect';
  static const forget = 'forget';
  static const send = 'send'; // {kind, b64} — fire-and-forget media/notif/icon
  static const readOtaInfo = 'readOtaInfo';
  static const bleOta = 'bleOta'; // {model}
  static const reloadFilter = 'reloadFilter';
  static const bleRestart = 'bleRestart'; // force a clean reconnect

  // File manager.
  static const fileList = 'fileList'; // {path}
  static const fileDownload = 'fileDownload'; // {path} → reply {tmpPath}
  static const fileUpload = 'fileUpload'; // {path, b64}
  static const fileUploadPath = 'fileUploadPath'; // {path, src}
  static const fileDelete = 'fileDelete'; // {path}
  static const fileMkdir = 'fileMkdir'; // {path}
  static const fileRename = 'fileRename'; // {src, dst}

  // LISP editor (VESC over a NUS link — head unit bridge or direct adapter).
  static const lispRead = 'lispRead'; // → reply {code, stats}
  static const lispUpload = 'lispUpload'; // {code, run} → reply {stats}
  static const lispRun = 'lispRun';
  static const lispStop = 'lispStop';
  static const lispStatsStart = 'lispStatsStart'; // begin GET_STATS polling
  static const lispStatsStop = 'lispStatsStop';
  static const lispStatsOnce = 'lispStatsOnce'; // → reply {stats}

  // Console: asynchronous `(print ...)` output from the running script.
  static const lispConsoleRead = 'lispConsoleRead'; // {sinceSeq, maxLines}
  static const lispConsoleClear = 'lispConsoleClear';
  static const lispConsoleSub = 'lispConsoleSub'; // {on} — live push on/off
  static const lispConsoleDebug = 'lispConsoleDebug'; // {on} → {histogram}

  // VESC BLE Helper (ESP32-C3) — a second GATT link alongside the head unit.
  static const helperConnect = 'helperConnect'; // {remoteId?}
  static const helperDisconnect = 'helperDisconnect';
  static const helperStatus = 'helperStatus'; // → {state, fw, busy}
  static const helperScan = 'helperScan'; // {what}
  static const helperBind = 'helperBind'; // {hit}
  static const helperUnbind = 'helperUnbind'; // {what}
  static const helperGetParams = 'helperGetParams';
  static const helperSetParams = 'helperSetParams'; // {b64}
  static const helperThrottle = 'helperThrottle'; // {value}
  static const helperGetBinding = 'helperGetBinding'; // {idx}
  static const helperSetBinding = 'helperSetBinding'; // {binding}
  static const helperOta = 'helperOta'; // {b64} — flash the helper firmware

  /// Pick the VESC target: `{kind: 'headUnit'|'direct', remoteId?, name?}`.
  static const vescSelect = 'vescSelect';

  /// Editor opened — re-apply the persisted target (connects a saved adapter).
  static const vescResume = 'vescResume';

  /// Editor closed — drop a direct adapter link, keep the saved choice.
  static const vescRelease = 'vescRelease';
}

/// task → UI: every message carries `t`.
class IpcEvt {
  static const state = 'state'; // {state, savedRemoteId, supportsFm, ...}
  static const resp = 'resp'; // {id, ...payload}  — success reply
  static const err = 'err'; // {id, kind, key?, notReady?, msg?} — failure
  static const progress = 'progress'; // {id, frac}
  static const otaUpload = 'otaUpload'; // {id, frac}
  static const otaVerify = 'otaVerify'; // {id}
  static const lispStats = 'lispStats'; // {stats} — unsolicited, while polling
  static const lispConsole = 'lispConsole'; // {chunk} — batched print output
  static const vescTarget = 'vescTarget'; // {target} — VESC link target changed

  // Helper (ESP32-C3), all unsolicited.
  static const helperState = 'helperState'; // {state, fw, busy}
  static const helperStatusFrame = 'helperStatusFrame'; // {status}
  static const helperParams = 'helperParams'; // {b64}
  static const helperBinding = 'helperBinding'; // {binding}
  static const helperScanHit = 'helperScanHit'; // {hit}
  static const helperLog = 'helperLog'; // {line}
  static const helperOtaProgress = 'helperOtaProgress'; // {frac}
}

/// Error `kind`s carried on an [IpcEvt.err]: lets the proxy rebuild the right
/// exception type for the UI.
class IpcErrKind {
  static const generic = 'generic';
  static const fileOp = 'fileOp'; // → FileOpException(key, notReady)
  static const lispOp = 'lispOp'; // → LispException(key)
  static const helperOp = 'helperOp'; // → HelperOpException(msg)
}
