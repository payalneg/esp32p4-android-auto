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

  // LISP editor (VESC over the NUS bridge).
  static const lispRead = 'lispRead'; // → reply {code}
  static const lispUpload = 'lispUpload'; // {code, run}
  static const lispRun = 'lispRun';
  static const lispStop = 'lispStop';
  static const lispStatsStart = 'lispStatsStart'; // begin GET_STATS polling
  static const lispStatsStop = 'lispStatsStop';
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
}

/// Error `kind`s carried on an [IpcEvt.err]: lets the proxy rebuild the right
/// exception type for the UI.
class IpcErrKind {
  static const generic = 'generic';
  static const fileOp = 'fileOp'; // → FileOpException(key, notReady)
  static const lispOp = 'lispOp'; // → LispException(key)
}
