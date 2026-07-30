/// Session persistence.
///
/// A session costs money and flash cycles, so losing it to a screen change is
/// not a cosmetic bug. These pin: it survives a round-trip, the transcript is
/// readable again afterwards, the system prompt is re-attached from the build
/// (not resurrected from disk), and "new chat" really forgets.
library;

import 'package:aa_bridge/agent/agent_events.dart';
import 'package:aa_bridge/agent/agent_prompt.dart';
import 'package:aa_bridge/agent/agent_session.dart';
import 'package:aa_bridge/agent/ai_client.dart';
import 'package:aa_bridge/agent/ai_models.dart';
import 'package:aa_bridge/agent/cancel_token.dart';
import 'package:aa_bridge/settings/agent_settings.dart';
import 'package:flutter/widgets.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:shared_preferences/shared_preferences.dart';

import 'fake_lisp_device.dart';

const _script = '(def x 0)\n@const-start\n(defun t () (sleep 1))\n@const-end\n';

class _Scripted extends AiClient {
  _Scripted(this.turns) : super(apiKey: 'test');
  final List<List<ChatEvent>> turns;
  int _i = 0;

  @override
  Stream<ChatEvent> stream(ChatRequest req,
      {CancelToken? cancel, int maxAttempts = 3}) async* {
    if (_i >= turns.length) {
      yield const ChatCompleted(
          finishReason: 'stop',
          content: 'done',
          toolCalls: [],
          usage: ChatUsage());
      return;
    }
    for (final e in turns[_i++]) {
      yield e;
    }
  }
}

List<ChatEvent> textTurn(String s) => [
      ContentDelta(s),
      ChatCompleted(
          finishReason: 'stop',
          content: s,
          toolCalls: const [],
          usage: const ChatUsage(promptTokens: 50, completionTokens: 5)),
    ];

List<ChatEvent> toolTurn(String name, String args, {String id = 'c1'}) => [
      ChatCompleted(
        finishReason: 'tool_calls',
        content: '',
        toolCalls: [ToolCall(id: id, name: name, argumentsJson: args)],
        usage: const ChatUsage(promptTokens: 50, completionTokens: 9),
      ),
    ];

void main() {
  TestWidgetsFlutterBinding.ensureInitialized();

  setUp(() async {
    SharedPreferences.setMockInitialValues({});
    await AgentSettings.instance.load();
    await AgentSettings.instance.setApiKey('test-key');
  });

  AgentSession make(_Scripted client, {TextEditingController? code}) =>
      AgentSession(
        device: FakeLispDevice(initialCode: _script),
        code: code ?? TextEditingController(),
        clientFactory: () => client,
      );

  test('a finished exchange survives into a new session object', () async {
    final a = make(_Scripted([textTurn('The gain is 0.02.')]));
    await a.send('what is the cruise gain?');
    await a.save();

    final b = make(_Scripted([]));
    expect(await b.restore(), isTrue);
    expect(b.events.whereType<UserMsgEvent>().single.text,
        'what is the cruise gain?');
    expect(b.events.whereType<AssistantMsgEvent>().single.text,
        'The gain is 0.02.');
  });

  test('tool steps come back with their outcome', () async {
    final a = make(_Scripted([
      toolTurn('read_lines', '{"start":1,"count":2}'),
      textTurn('Line 1 defines x.'),
    ]));
    await a.send('show me the top');
    await a.save();

    final b = make(_Scripted([]));
    await b.restore();
    final step = b.events.whereType<ToolStepEvent>().single;
    expect(step.name, 'read_lines');
    expect(step.status, StepStatus.ok);
    expect(step.resultJson, contains('total_lines'));
  });

  test('the model keeps its context, so it can carry on', () async {
    final a = make(_Scripted([textTurn('noted')]));
    await a.send('remember: the wheel is 20 inches');
    await a.save();

    final b = make(_Scripted([]));
    await b.restore();
    final userMsgs = b.transcript
        .where((m) => m.role == ChatRole.user)
        .map((m) => m.content)
        .join('\n');
    expect(userMsgs, contains('20 inches'));
  });

  test('the system prompt is re-attached from the build, not from disk',
      () async {
    final a = make(_Scripted([textTurn('hi')]));
    await a.send('hello');
    await a.save();

    final b = make(_Scripted([]));
    await b.restore();
    final system = b.transcript.first;
    expect(system.role, ChatRole.system);
    expect(system.content, kAgentSystemPrompt);
    // Saved payload must not carry the (large, constant) prompt around.
    expect(a.toJson().toString(), isNot(contains('coding agent embedded')));
  });

  test('spend and step counters survive', () async {
    final a = make(_Scripted([textTurn('one'), textTurn('two')]));
    await a.send('first');
    await a.save();

    final b = make(_Scripted([]));
    await b.restore();
    expect(b.progress.step, greaterThan(0));
    expect(b.progress.usage.promptTokens, greaterThan(0));
  });

  test('nothing to restore is not an error', () async {
    final s = make(_Scripted([]));
    expect(await s.restore(), isFalse);
    expect(s.events, isEmpty);
  });

  test('new chat forgets, on screen and on disk', () async {
    final a = make(_Scripted([textTurn('hi')]));
    await a.send('hello');
    await a.save();
    await a.clear();

    expect(a.events, isEmpty);
    expect(a.progress.step, 0);
    final b = make(_Scripted([]));
    expect(await b.restore(), isFalse);
  });

  test('the working copy is left alone by clear()', () async {
    final code = TextEditingController(text: _script);
    final a = make(_Scripted([textTurn('hi')]), code: code);
    await a.send('hello');
    await a.clear();
    expect(code.text, _script);
  });

  test('an oversized transcript is trimmed to the tail', () async {
    final a = make(_Scripted([]));
    for (var i = 0; i < 90; i++) {
      await a.send('message $i');
    }
    final saved = (a.toJson()['messages'] as List).length;
    expect(saved, lessThanOrEqualTo(60));
    // The tail is what matters — the newest exchange must be in there.
    expect(a.toJson().toString(), contains('message 89'));
  });
}
