/// End-to-end agent loop against a scripted model and the in-memory VESC.
///
/// No network, no hardware: the whole user → model → tool → device →
/// observation cycle runs in CI, including the paths that only appear when
/// things go wrong (a declined flash, a backgrounded app, an exhausted
/// budget).
library;

import 'package:aa_bridge/agent/agent_events.dart';
import 'package:aa_bridge/agent/agent_session.dart';
import 'package:aa_bridge/agent/ai_client.dart';
import 'package:aa_bridge/agent/ai_models.dart';
import 'package:aa_bridge/agent/cancel_token.dart';
import 'package:aa_bridge/settings/agent_settings.dart';
import 'package:flutter/widgets.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:shared_preferences/shared_preferences.dart';

import 'fake_lisp_device.dart';

const _script = '(def x 0)\n'
    '@const-start\n'
    '(defun tick () { (setq x (+ x 1)) (sleep 1) })\n'
    '(spawn tick)\n'
    '@const-end\n';

/// A model whose turns are decided in advance.
class ScriptedClient extends AiClient {
  ScriptedClient(this.turns) : super(apiKey: 'test');

  final List<List<ChatEvent>> turns;
  final requests = <ChatRequest>[];
  int _i = 0;

  @override
  Stream<ChatEvent> stream(ChatRequest req,
      {CancelToken? cancel, int maxAttempts = 3}) async* {
    requests.add(req);
    if (_i >= turns.length) {
      // Ran off the end of the script: end the conversation cleanly rather
      // than looping forever.
      yield const ChatCompleted(
          finishReason: 'stop',
          content: 'done',
          toolCalls: [],
          usage: ChatUsage());
      return;
    }
    for (final e in turns[_i++]) {
      cancel?.throwIfCancelled();
      yield e;
    }
  }
}

/// A model that always fails the same way.
class FailingClient extends AiClient {
  FailingClient(this.error) : super(apiKey: 'test');
  final DeepSeekError error;

  @override
  Stream<ChatEvent> stream(ChatRequest req,
      {CancelToken? cancel, int maxAttempts = 3}) async* {
    throw error;
  }
}

List<ChatEvent> textTurn(String text) => [
      ContentDelta(text),
      ChatCompleted(
          finishReason: 'stop',
          content: text,
          toolCalls: const [],
          usage: const ChatUsage(promptTokens: 100, completionTokens: 10)),
    ];

List<ChatEvent> toolTurn(String name, String args, {String id = 'c1'}) => [
      ChatCompleted(
        finishReason: 'tool_calls',
        content: '',
        toolCalls: [ToolCall(id: id, name: name, argumentsJson: args)],
        usage: const ChatUsage(promptTokens: 100, completionTokens: 20),
      ),
    ];

List<ChatEvent> get finishTurn =>
    toolTurn('finish', '{"status":"solved","summary":"done"}', id: 'fin');

void main() {
  TestWidgetsFlutterBinding.ensureInitialized();

  setUp(() async {
    SharedPreferences.setMockInitialValues({});
    await AgentSettings.instance.load();
    // Secure storage has no platform side in tests; the setter keeps the key
    // in memory regardless, which is all the session needs.
    await AgentSettings.instance.setApiKey('test-key');
    await AgentSettings.instance.setMaxSteps(24);
  });

  AgentSession make(ScriptedClient client,
      {FakeLispDevice? device, TextEditingController? code}) {
    return AgentSession(
      device: device ?? FakeLispDevice(initialCode: _script),
      code: code ?? TextEditingController(),
      clientFactory: () => client,
    );
  }

  test('a plain answer ends the turn', () async {
    final s = make(ScriptedClient([textTurn('The gain is 0.02.')]));
    await s.send('what is the cruise gain?');

    expect(s.state, AgentRunState.done);
    expect(s.events.whereType<UserMsgEvent>(), hasLength(1));
    expect(s.events.whereType<AssistantMsgEvent>().single.text,
        'The gain is 0.02.');
  });

  test('a tool call runs and its result goes back to the model', () async {
    final dev = FakeLispDevice(initialCode: _script);
    final client = ScriptedClient([
      toolTurn('read_script', '{}'),
      finishTurn,
    ]);
    final s = make(client, device: dev);
    await s.send('read the script');

    final step = s.events.whereType<ToolStepEvent>().first;
    expect(step.name, 'read_script');
    expect(step.status, StepStatus.ok);
    expect(dev.calls, contains('read'));

    // The observation is in the transcript as a tool message (the first one —
    // the `finish` call adds another).
    final toolMsg =
        s.transcript.firstWhere((m) => m.role == ChatRole.tool);
    expect(toolMsg.toolCallId, 'c1');
    expect(toolMsg.content, contains('"ok":true'));
  });

  test('edits land in the editor buffer as they happen', () async {
    final code = TextEditingController(text: _script);
    final client = ScriptedClient([
      toolTurn(
          'apply_patch',
          '{"intent":"slow it down","patch":"<<<<<<< SEARCH\\n(sleep 1)\\n'
              '=======\\n(sleep 5)\\n>>>>>>> REPLACE"}'),
      finishTurn,
    ]);
    final s = make(client, code: code);
    await s.send('make the tick slower');

    expect(code.text, contains('(sleep 5)'));
    expect(s.events.whereType<ToolStepEvent>().first.diff, contains('+'));
  });

  test('finish ends the loop even with turns left', () async {
    final client = ScriptedClient([
      finishTurn,
      textTurn('this should never be requested'),
    ]);
    final s = make(client);
    await s.send('do it');

    expect(s.state, AgentRunState.done);
    expect(client.requests, hasLength(1));
  });

  test('a bad-JSON tool call is an observation, not a crash', () async {
    final client = ScriptedClient([
      toolTurn('read_lines', '{"start": '), // truncated
      finishTurn,
    ]);
    final s = make(client);
    await s.send('show me line 1');

    final step = s.events.whereType<ToolStepEvent>().first;
    expect(step.status, StepStatus.failed);
    expect(step.resultJson, contains('bad_arguments'));
    expect(s.state, AgentRunState.done);
  });

  test('an unknown tool is reported back rather than throwing', () async {
    final client = ScriptedClient([
      toolTurn('set_motor_current', '{"amps": 50}'),
      finishTurn,
    ]);
    final s = make(client);
    await s.send('spin the motor');

    expect(s.events.whereType<ToolStepEvent>().first.resultJson,
        contains('unknown_tool'));
  });

  test('flashing waits for the user and proceeds when approved', () async {
    final dev = FakeLispDevice(initialCode: _script)..movingGlobals.add('x');
    final code = TextEditingController(text: _script);
    final client = ScriptedClient([
      toolTurn('flash_script',
          '{"run":true,"rationale":"apply the change","moving_globals":["x"]}'),
      finishTurn,
    ]);
    final s = make(client, device: dev, code: code);

    final run = s.send('flash it');
    // The loop parks on the confirmation card.
    await _until(() => s.events.whereType<ConfirmEvent>().isNotEmpty);
    expect(s.state, AgentRunState.awaitingUser);
    expect(dev.calls, isEmpty);

    final confirm = s.events.whereType<ConfirmEvent>().single;
    expect(confirm.run, isTrue);
    expect(confirm.rationale, 'apply the change');
    s.resolveConfirmation(confirm.callId, true);

    await run;
    expect(confirm.status, StepStatus.ok);
    expect(dev.stored, _script);
    expect(s.hasFlashed, isTrue);
    final step = s.events.whereType<ToolStepEvent>()
        .firstWhere((e) => e.name == 'flash_script');
    expect(step.verify?['verdict'], 'ok');
  });

  test('declining a flash leaves the device untouched', () async {
    final dev = FakeLispDevice(initialCode: _script);
    final client = ScriptedClient([
      toolTurn('flash_script', '{"run":true,"rationale":"go"}'),
      finishTurn,
    ]);
    final s = make(client,
        device: dev, code: TextEditingController(text: _script));

    final run = s.send('flash it');
    await _until(() => s.events.whereType<ConfirmEvent>().isNotEmpty);
    s.resolveConfirmation(
        s.events.whereType<ConfirmEvent>().single.callId, false);
    await run;

    expect(dev.calls, isEmpty);
    expect(dev.running, isFalse);
    expect(
        s.events
            .whereType<ToolStepEvent>()
            .firstWhere((e) => e.name == 'flash_script')
            .resultJson,
        contains('user_declined'));
  });

  test('backgrounded, nothing reaches the device', () async {
    final dev = FakeLispDevice(initialCode: _script);
    final client = ScriptedClient([
      toolTurn('read_script', '{}'),
      finishTurn,
    ]);
    final s = make(client, device: dev)..setBackgrounded(true);
    await s.send('read it');

    expect(dev.calls, isEmpty);
    expect(s.events.whereType<ToolStepEvent>().first.resultJson,
        contains('paused'));
  });

  test('the step budget forces a final summary instead of cutting off',
      () async {
    await AgentSettings.instance.setMaxSteps(1);
    final client = ScriptedClient([
      finishTurn,
      finishTurn,
    ]);
    final s = make(client);
    await s.send('do something long');

    expect(s.events.whereType<NoticeEvent>().any((e) => e.kind == NoticeKind.budget),
        isTrue);
    // The forced turn pins the model to `finish`.
    final choice = client.requests.first.toolChoice;
    expect(choice, isA<Map>());
    expect((choice! as Map)['function'], containsPair('name', 'finish'));
  });

  test('an API error surfaces as a localizable notice', () async {
    final s = AgentSession(
      device: FakeLispDevice(),
      code: TextEditingController(),
      clientFactory: () => FailingClient(const DsAuthError('bad key')),
    );
    await s.send('hello');

    expect(s.state, AgentRunState.failed);
    final notice = s.events.whereType<NoticeEvent>().last;
    expect(notice.kind, NoticeKind.error);
    expect(notice.messageKey, 'agent.err.auth');
  });

  test('without a key the session refuses to start', () async {
    await AgentSettings.instance.setApiKey('');
    final s = make(ScriptedClient([textTurn('hi')]));
    await s.send('hello');

    expect(s.events.whereType<NoticeEvent>().single.messageKey,
        'agent.err.nokey');
    expect(s.events.whereType<AssistantMsgEvent>(), isEmpty);
  });

  test('the system prompt is byte-identical across turns (cache prefix)',
      () async {
    final client = ScriptedClient([
      toolTurn('read_lines', '{"start":1}'),
      finishTurn,
    ]);
    final s = make(client);
    await s.send('look at it');

    expect(client.requests.length, greaterThan(1));
    final first = client.requests.first.messages.first;
    final second = client.requests[1].messages.first;
    expect(first.role, ChatRole.system);
    expect(second.content, first.content);
    // …and the system prompt carries the rules that fail silently.
    expect(first.content, contains('@const'));
    expect(first.content, contains('18 bindings'));
  });

  test('emergency stop halts the script', () async {
    final dev = FakeLispDevice()..running = true;
    final s = make(ScriptedClient([]), device: dev);
    await s.emergencyStop();

    expect(dev.running, isFalse);
    expect(s.events.whereType<NoticeEvent>().last.messageKey,
        'agent.stopped.script');
  });
}

/// Poll until [cond] holds — the loop parks on a Completer, so there is no
/// event to await directly.
Future<void> _until(bool Function() cond,
    {Duration timeout = const Duration(seconds: 5)}) async {
  final sw = Stopwatch()..start();
  while (!cond()) {
    if (sw.elapsed > timeout) fail('condition not met within $timeout');
    await Future<void>.delayed(const Duration(milliseconds: 5));
  }
}
