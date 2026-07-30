/// Transcript entries for the Assistant tab.
///
/// These are mutable on purpose: a streaming answer appends into the same
/// entry and the tab re-renders only the tail, rather than allocating an
/// event per token.
library;

import 'ai_models.dart';

enum StepStatus { running, ok, failed, awaitingUser, declined, cancelled }

enum NoticeKind { error, budget, info }

sealed class AgentEvent {
  AgentEvent(this.id);
  final int id;
}

class UserMsgEvent extends AgentEvent {
  UserMsgEvent(super.id, this.text);
  final String text;
}

class AssistantMsgEvent extends AgentEvent {
  AssistantMsgEvent(super.id);
  final StringBuffer _buf = StringBuffer();
  bool done = false;
  void append(String s) => _buf.write(s);
  String get text => _buf.toString();
  bool get isEmpty => _buf.isEmpty;
}

/// Thinking-mode output. Shown collapsed and never sent back to the API.
class ThinkingEvent extends AgentEvent {
  ThinkingEvent(super.id);
  final StringBuffer _buf = StringBuffer();
  bool done = false;
  void append(String s) => _buf.write(s);
  String get text => _buf.toString();
}

/// One tool invocation — the Claude-Code-style step line.
class ToolStepEvent extends AgentEvent {
  ToolStepEvent(super.id,
      {required this.callId, required this.name, required this.argsJson});

  final String callId;
  final String name;
  final String argsJson;

  StepStatus status = StepStatus.running;

  /// One-line summary rendered next to the tool name.
  String? summary;

  /// Full result, pretty-printed, shown when the step is expanded.
  String? resultJson;

  /// Set by editing tools so the UI can render the change instead of JSON.
  String? diff;

  /// Set by flash_script: the post-flash verification report.
  Map<String, dynamic>? verify;

  bool get touchedDevice => kDeviceTools.contains(name);
}

/// A device-mutating call waiting for the user to tap. Rendered inline in the
/// transcript rather than as a dialog: a modal over a streaming transcript is
/// easy to miss, and the card stays as a record of what was approved.
class ConfirmEvent extends AgentEvent {
  ConfirmEvent(
    super.id, {
    required this.callId,
    required this.titleKey,
    required this.rationale,
    required this.diffSummary,
    required this.warnings,
    required this.run,
  });

  final String callId;
  final String titleKey;
  final String rationale;
  final String diffSummary;
  final List<String> warnings;

  /// Whether the script will be started after writing.
  final bool run;

  StepStatus status = StepStatus.awaitingUser;
}

class NoticeEvent extends AgentEvent {
  NoticeEvent(super.id,
      {required this.kind, required this.messageKey, this.detail, this.args});
  final NoticeKind kind;
  final String messageKey;
  final String? detail;
  final Map<String, String>? args;
}

/// Header state: which step we're on and what it has cost.
class AgentProgress {
  const AgentProgress({
    this.step = 0,
    this.maxSteps = 0,
    this.usage = const ChatUsage(),
    this.costUsd,
    this.flashes = 0,
    this.maxFlashes = 0,
  });

  final int step;
  final int maxSteps;
  final ChatUsage usage;

  /// null when the provider didn't report a cost and the model isn't in the
  /// local price table — the UI shows a dash, never a guess.
  final double? costUsd;
  final int flashes;
  final int maxFlashes;
}

/// Tools that touch the VESC. Used for the "device" styling in the UI and to
/// decide whether a step may run while the app is backgrounded.
const kDeviceTools = {
  'read_script',
  'get_stats',
  'read_console',
  'flash_script',
  'set_running',
  'revert_to_flashed_backup',
};
