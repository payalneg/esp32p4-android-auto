/// The agent loop: user turn → model → tool calls → device → observations →
/// model, until it calls `finish` or runs out of budget.
///
/// It lives in the UI isolate, not the BLE one. Everything it needs is already
/// reachable through [LispProxy]; the loop is fundamentally interactive
/// (streaming text, tool cards, blocking on a tap mid-tool-call), and a motor
/// controller should not be reflashed while nobody is looking at the screen.
/// Pausing when backgrounded is a feature, not a limitation.
library;

import 'dart:async';
import 'dart:convert';

import 'package:flutter/foundation.dart';
import 'package:flutter/widgets.dart' show TextEditingController;
import 'package:shared_preferences/shared_preferences.dart';

import '../settings/agent_settings.dart';
import 'agent_budget.dart';
import 'agent_events.dart';
import 'agent_prompt.dart';
import 'agent_tools.dart';
import 'ai_client.dart';
import 'ai_models.dart';
import 'cancel_token.dart';
import 'lisp_device.dart';

enum AgentRunState {
  idle,
  thinking,
  streaming,
  toolRunning,
  awaitingUser,
  paused,
  cancelling,
  done,
  failed,
}

class AgentSession {
  AgentSession({
    required this.device,
    required this.code,
    AgentSettings? settings,
    AiClient Function()? clientFactory,
  })  : _settings = settings ?? AgentSettings.instance,
        _clientFactory = clientFactory;

  final LispDevice device;

  /// The working copy IS the editor's buffer — one source of truth, so edits
  /// show up in the Code tab as they happen and the user can hand-edit
  /// between steps.
  final TextEditingController code;

  final AgentSettings _settings;
  final AiClient Function()? _clientFactory;

  final _events = <AgentEvent>[];
  final _changes = StreamController<void>.broadcast();
  final _messages = <ChatMessage>[];
  final _counters = AgentUsageCounters();
  final _pendingConfirms = <String, Completer<bool>>{};

  int _nextEventId = 1;
  CancelToken? _cancel;
  AgentRunState _state = AgentRunState.idle;
  AgentProgress _progress = const AgentProgress();
  ChatUsage _usage = const ChatUsage();
  double _spent = 0;
  bool _backgrounded = false;
  bool _finished = false;
  ToolCtx? _ctx;
  late ToolRegistry _tools;

  List<AgentEvent> get events => List.unmodifiable(_events);

  /// Fires whenever the transcript or header state changed. Deliberately a
  /// bare signal: the tab reads [events] and rebuilds its tail.
  Stream<void> get changes => _changes.stream;

  AgentRunState get state => _state;
  AgentProgress get progress => _progress;
  bool get busy =>
      _state != AgentRunState.idle &&
      _state != AgentRunState.done &&
      _state != AgentRunState.failed;

  /// True once anything has been written to the VESC this session — the STOP
  /// button appears from that point on.
  bool get hasFlashed => (_ctx?.counters.flashes ?? 0) > 0;

  AgentBudget get _budget => AgentBudget(
        maxSteps: _settings.maxSteps,
        maxFlashes: _settings.maxFlashes,
        maxSpendUsd: _settings.spendCap,
      );

  void _touch() {
    if (!_changes.isClosed) _changes.add(null);
  }

  void _setState(AgentRunState s) {
    _state = s;
    _touch();
  }

  T _add<T extends AgentEvent>(T e) {
    _events.add(e);
    _touch();
    return e;
  }

  /// The app went to / came back from the background. An in-flight step is
  /// allowed to finish — tearing down an HTTP stream or a BLE transfer
  /// mid-way is worse than letting it complete — but the loop then stops
  /// before starting anything that touches the device.
  void setBackgrounded(bool v) {
    _backgrounded = v;
    if (!v && _state == AgentRunState.paused) _setState(AgentRunState.idle);
  }

  void cancel() {
    if (_cancel == null) return;
    _setState(AgentRunState.cancelling);
    _cancel!.cancel();
    // Any tool waiting on a tap must not hang forever.
    for (final c in _pendingConfirms.values) {
      if (!c.isCompleted) c.complete(false);
    }
    _pendingConfirms.clear();
  }

  /// Emergency stop: cancel the loop and halt the script. Stopping is the safe
  /// direction, so it never asks. The `_serial` queue in the link layer is
  /// FIFO, so if a 23 KB upload is in flight this waits for it — the UI says
  /// so rather than pretending the stop is instant.
  Future<void> emergencyStop() async {
    cancel();
    try {
      await device.setRunning(false);
      _add(NoticeEvent(_nextEventId++,
          kind: NoticeKind.info, messageKey: 'agent.stopped.script'));
    } catch (e) {
      _add(NoticeEvent(_nextEventId++,
          kind: NoticeKind.error,
          messageKey: 'agent.err.stopFailed',
          detail: '$e'));
    }
  }

  void resolveConfirmation(String callId, bool approved) {
    final c = _pendingConfirms.remove(callId);
    if (c != null && !c.isCompleted) c.complete(approved);
    for (final e in _events.reversed) {
      if (e is ConfirmEvent && e.callId == callId) {
        e.status = approved ? StepStatus.ok : StepStatus.declined;
        break;
      }
    }
    _touch();
  }

  Future<bool> _confirm(ConfirmRequest req) {
    final ev = _add(ConfirmEvent(
      _nextEventId++,
      callId: req.callId,
      titleKey: req.titleKey,
      rationale: req.rationale,
      diffSummary: req.diffSummary,
      warnings: req.warnings,
      run: req.run,
    ));
    final c = Completer<bool>();
    _pendingConfirms[req.callId] = c;
    _setState(AgentRunState.awaitingUser);

    // Don't hang the session on a user who walked away.
    Timer(const Duration(minutes: 5), () {
      if (!c.isCompleted) {
        ev.status = StepStatus.declined;
        _pendingConfirms.remove(req.callId);
        c.complete(false);
        _touch();
      }
    });
    return c.future;
  }

  // ---- the loop ----------------------------------------------------------

  Future<void> send(String userText) async {
    if (busy) return;
    if (!_settings.configured) {
      _add(NoticeEvent(_nextEventId++,
          kind: NoticeKind.error, messageKey: 'agent.err.nokey'));
      return;
    }

    final cancel = CancelToken();
    _cancel = cancel;
    _tools = ToolRegistry(writeEnabled: true);
    _counters.clock.start();

    _ctx ??= ToolCtx(
      dev: device,
      counters: _counters,
      budget: _budget,
      cancel: cancel,
      confirm: _confirm,
      onProgress: (_) {},
      getWorking: () => code.text,
      setWorking: (s) => code.text = s,
    );

    if (_messages.isEmpty) {
      _messages
        ..add(const ChatMessage.system(kAgentSystemPrompt))
        ..add(ChatMessage.user(sessionContext(
          linkLabel: device.linkLabel,
          consoleAvailable: _ctx!.consoleEverAlive,
          maxFlashes: _budget.maxFlashes,
        )));
    }
    _messages.add(ChatMessage.user(userText));
    _add(UserMsgEvent(_nextEventId++, userText));

    final client = _clientFactory?.call() ??
        AiClient(
          apiKey: _settings.apiKey ?? '',
          provider: _settings.provider,
          baseUrl: _settings.baseUrl,
          strict: _settings.strict,
        );

    try {
      await _runLoop(client, cancel);
    } on AgentCancelled {
      _add(NoticeEvent(_nextEventId++,
          kind: NoticeKind.info, messageKey: 'agent.cancelled'));
      _setState(AgentRunState.idle);
    } on DeepSeekError catch (e) {
      _add(NoticeEvent(_nextEventId++,
          kind: NoticeKind.error, messageKey: e.i18nKey, detail: e.detail));
      _setState(AgentRunState.failed);
    } catch (e) {
      _add(NoticeEvent(_nextEventId++,
          kind: NoticeKind.error,
          messageKey: 'agent.err.internal',
          detail: '$e'));
      _setState(AgentRunState.failed);
    } finally {
      _cancel = null;
      unawaited(_settings.addSpend(_spent));
      unawaited(save());
    }
  }

  Future<void> _runLoop(AiClient client, CancelToken cancel) async {
    final budget = _budget;
    _finished = false;

    for (var step = 0;; step++) {
      cancel.throwIfCancelled();
      _counters.steps = step + 1;
      _publishProgress();

      final exhausted = budget.exhaustedKey(_counters);
      final forcedFinish = exhausted != null;
      if (forcedFinish) {
        _add(NoticeEvent(_nextEventId++,
            kind: NoticeKind.budget, messageKey: exhausted));
      }

      final completed = await _oneTurn(
        client,
        cancel,
        forceFinish: forcedFinish,
      );

      _messages.add(completed.message);

      if (!completed.wantsTools) {
        _setState(AgentRunState.done);
        return;
      }

      for (final call in completed.toolCalls) {
        cancel.throwIfCancelled();
        await _dispatch(call, cancel);
        if (_finished) {
          _setState(AgentRunState.done);
          return;
        }
      }
      // Checkpoint after every completed step: a flash cycle costs minutes,
      // and losing the conversation to a backgrounded app would mean paying
      // for it twice.
      unawaited(save());

      if (forcedFinish) {
        _setState(AgentRunState.done);
        return;
      }
    }
  }

  Future<ChatCompleted> _oneTurn(
    AiClient client,
    CancelToken cancel, {
    required bool forceFinish,
  }) async {
    _setState(_settings.thinking
        ? AgentRunState.thinking
        : AgentRunState.streaming);

    AssistantMsgEvent? msg;
    ThinkingEvent? thinking;
    ChatCompleted? completed;

    final req = ChatRequest(
      provider: _settings.provider,
      model: _settings.model,
      messages: _messages,
      tools: _tools.schemas(
        consoleAvailable: _ctx?.consoleEverAlive ?? true,
        strict: _settings.strict && _settings.provider != AiProvider.deepseek,
      ),
      temperature: _settings.temperature,
      thinking: _settings.thinking,
      toolChoice: forceFinish
          ? {
              'type': 'function',
              'function': {'name': 'finish'}
            }
          : null,
    );

    await for (final e in client.stream(req, cancel: cancel)) {
      switch (e) {
        case ReasoningDelta(:final text):
          thinking ??= _add(ThinkingEvent(_nextEventId++));
          thinking.append(text);
          _touch();
        case ContentDelta(:final text):
          thinking?.done = true;
          msg ??= _add(AssistantMsgEvent(_nextEventId++));
          msg.append(text);
          _setState(AgentRunState.streaming);
          _touch();
        case ChatCompleted():
          completed = e;
      }
    }

    thinking?.done = true;
    msg?.done = true;
    if (msg != null && msg.isEmpty) _events.remove(msg);

    if (completed == null) throw const DsBadResponse('no completion');
    _usage = _usage + completed.usage;
    final cost = estimateCost(_settings.model, completed.usage);
    if (cost != null) _spent += cost;
    _publishProgress();
    return completed;
  }

  Future<void> _dispatch(ToolCall call, CancelToken cancel) async {
    final spec = _tools[call.name];
    final step = _add(ToolStepEvent(
      _nextEventId++,
      callId: call.id,
      name: call.name,
      argsJson: call.argumentsJson,
    ));
    _counters.toolCalls++;
    _setState(AgentRunState.toolRunning);

    Map<String, dynamic> result;
    if (spec == null) {
      result = {
        'ok': false,
        'error': 'unknown_tool',
        'detail': 'No tool named ${call.name}.',
      };
    } else if (_backgrounded && spec.touchesDevice) {
      // The user isn't looking at the screen; nothing goes to the motor
      // controller until they are.
      _setState(AgentRunState.paused);
      result = {
        'ok': false,
        'error': 'paused',
        'detail': 'The app is in the background; device access is paused.',
      };
    } else {
      Map<String, dynamic> args;
      try {
        final decoded = jsonDecode(
            call.argumentsJson.trim().isEmpty ? '{}' : call.argumentsJson);
        args = decoded is Map
            ? decoded.cast<String, dynamic>()
            : <String, dynamic>{};
      } catch (e) {
        args = <String, dynamic>{};
        result = {
          'ok': false,
          'error': 'bad_arguments',
          'detail': 'Arguments were not valid JSON: $e',
        };
        _finishStep(step, call, result);
        return;
      }
      try {
        result = await spec.run(_ctx!, args, call.id);
      } on AgentCancelled {
        step.status = StepStatus.cancelled;
        rethrow;
      } catch (e) {
        // A tool must never throw at the model — the failure is an
        // observation it can act on.
        result = {'ok': false, 'error': 'tool_crashed', 'detail': '$e'};
      }
      if (call.name == 'finish' && result['ok'] == true) _finished = true;
    }

    _finishStep(step, call, result);
  }

  void _finishStep(
      ToolStepEvent step, ToolCall call, Map<String, dynamic> result) {
    final ok = result['ok'] == true;
    step
      ..status = ok ? StepStatus.ok : StepStatus.failed
      ..resultJson = const JsonEncoder.withIndent('  ').convert(result)
      ..summary = _summarise(call.name, result)
      ..diff = result['diff'] as String?
      ..verify = result.containsKey('verdict') ? result : null;
    _messages.add(ChatMessage.tool(call.id, jsonEncode(result)));
    _publishProgress();
    _touch();
  }

  String _summarise(String tool, Map<String, dynamic> r) {
    if (r['ok'] != true) return '${r['error'] ?? 'failed'}';
    return switch (tool) {
      'read_script' => '${r['lines']} lines, ${r['bytes']} B',
      'read_lines' => 'lines ${r['start']}–${r['end']} of ${r['total_lines']}',
      'grep_script' => '${(r['matches'] as List?)?.length ?? 0} matches',
      'lint_script' => '${(r['errors'] as List?)?.length ?? 0} errors, '
          '${(r['warnings'] as List?)?.length ?? 0} warnings',
      'get_stats' => '${(r['samples'] as List?)?.length ?? 0} samples',
      'read_console' => '${(r['lines'] as List?)?.length ?? 0} lines',
      'apply_patch' => '${r['applied']} blocks applied',
      'write_script' => '${r['new_lines']} lines',
      'flash_script' ||
      'revert_to_flashed_backup' =>
        '${r['verdict']}${r['pass'] == true ? '' : ' — failed'}',
      'set_running' => r['running'] == true ? 'started' : 'stopped',
      'finish' => '',
      _ => '',
    };
  }

  void _publishProgress() {
    _progress = AgentProgress(
      step: _counters.steps,
      maxSteps: _budget.maxSteps,
      usage: _usage,
      costUsd: _usage.reportedCostUsd ??
          (kPricing.containsKey(_settings.model) ? _spent : null),
      flashes: _counters.flashes,
      maxFlashes: _budget.maxFlashes,
    );
    _touch();
  }

  // ---- persistence -------------------------------------------------------
  //
  // The conversation is worth keeping: a session costs real money and real
  // flash cycles, and leaving the editor (or Android killing the UI) used to
  // throw it all away. Saved after every completed step.
  //
  // The system prompt is NOT saved — it is a compile-time constant and gets
  // re-attached on restore, so an app update automatically upgrades the rules
  // of a restored conversation instead of resurrecting the old ones.

  static const _prefKey = 'agent_session_v1';

  /// Keep the tail only. The whole point is to be able to carry on, not to
  /// archive; an unbounded transcript would eventually blow the prefs entry.
  static const _maxSavedMessages = 60;
  static const _maxSavedChars = 256 * 1024;

  Map<String, dynamic> toJson() {
    final body = _messages.where((m) => m.role != ChatRole.system).toList();
    final tail = body.length > _maxSavedMessages
        ? body.sublist(body.length - _maxSavedMessages)
        : body;
    return {
      'v': 1,
      'messages': [for (final m in tail) m.toJson()],
      'steps': _counters.steps,
      'toolCalls': _counters.toolCalls,
      'flashes': _counters.flashes,
      'spent': _spent,
      'usage': _usage.toJson(),
    };
  }

  Future<void> save() async {
    try {
      if (_messages.where((m) => m.role != ChatRole.system).isEmpty) return;
      final text = jsonEncode(toJson());
      if (text.length > _maxSavedChars) return; // pathological; skip silently
      final p = await SharedPreferences.getInstance();
      await p.setString(_prefKey, text);
    } catch (e) {
      // Persistence is a convenience — never let it break a running session.
      debugPrint('[agent] session save failed: $e');
    }
  }

  /// Reload the previous conversation, if any. Returns true when something
  /// was restored.
  Future<bool> restore() async {
    try {
      final p = await SharedPreferences.getInstance();
      final text = p.getString(_prefKey);
      if (text == null || text.isEmpty) return false;
      final m = jsonDecode(text) as Map<String, dynamic>;
      final raw = (m['messages'] as List?) ?? const [];
      if (raw.isEmpty) return false;

      _messages
        ..clear()
        ..add(const ChatMessage.system(kAgentSystemPrompt))
        ..addAll([
          for (final e in raw)
            ChatMessage.fromJson((e as Map).cast<String, dynamic>())
        ]);
      _counters
        ..steps = (m['steps'] as num?)?.toInt() ?? 0
        ..toolCalls = (m['toolCalls'] as num?)?.toInt() ?? 0
        ..flashes = (m['flashes'] as num?)?.toInt() ?? 0;
      _spent = (m['spent'] as num?)?.toDouble() ?? 0;
      final u = m['usage'];
      if (u is Map) _usage = ChatUsage.fromJson(u.cast<String, dynamic>());

      _rebuildEvents();
      _publishProgress();
      return _events.isNotEmpty;
    } catch (e) {
      debugPrint('[agent] session restore failed: $e');
      return false;
    }
  }

  /// Start over: forget the conversation on disk and on screen. The working
  /// copy in the editor is left alone — that is the user's code.
  Future<void> clear() async {
    cancel();
    _messages.clear();
    _events.clear();
    _counters
      ..steps = 0
      ..toolCalls = 0
      ..flashes = 0;
    _spent = 0;
    _usage = const ChatUsage();
    _ctx = null;
    _setState(AgentRunState.idle);
    _publishProgress();
    try {
      final p = await SharedPreferences.getInstance();
      await p.remove(_prefKey);
    } catch (_) {}
  }

  /// Turn a restored transcript back into something readable. Tool results are
  /// matched to their calls by id so the step lines keep their outcome.
  void _rebuildEvents() {
    _events.clear();
    final results = <String, Map<String, dynamic>>{};
    for (final m in _messages) {
      if (m.role != ChatRole.tool || m.toolCallId == null) continue;
      try {
        final decoded = jsonDecode(m.content ?? '{}');
        if (decoded is Map) {
          results[m.toolCallId!] = decoded.cast<String, dynamic>();
        }
      } catch (_) {}
    }

    for (final m in _messages) {
      switch (m.role) {
        case ChatRole.system:
        case ChatRole.tool:
          break;
        case ChatRole.user:
          // The first user message is the session-context blurb, not the user.
          if (_events.isEmpty &&
              (m.content ?? '').startsWith('Session context:')) {
            break;
          }
          _add(UserMsgEvent(_nextEventId++, m.content ?? ''));
        case ChatRole.assistant:
          final text = m.content ?? '';
          if (text.isNotEmpty) {
            final e = _add(AssistantMsgEvent(_nextEventId++))..append(text);
            e.done = true;
          }
          for (final call in m.toolCalls) {
            final r = results[call.id];
            final step = _add(ToolStepEvent(_nextEventId++,
                callId: call.id,
                name: call.name,
                argsJson: call.argumentsJson));
            if (r == null) {
              step.status = StepStatus.cancelled;
            } else {
              step
                ..status =
                    r['ok'] == true ? StepStatus.ok : StepStatus.failed
                ..resultJson =
                    const JsonEncoder.withIndent('  ').convert(r)
                ..summary = _summarise(call.name, r)
                ..diff = r['diff'] as String?
                ..verify = r.containsKey('verdict') ? r : null;
            }
          }
      }
    }
  }

  Future<void> dispose() async {
    cancel();
    await save();
    await _changes.close();
  }

  @visibleForTesting
  List<ChatMessage> get transcript => List.unmodifiable(_messages);
}
