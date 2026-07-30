/// SSE decoding against recorded-shape fixtures. These are the cases that
/// break naive implementations: tool-call arguments split across chunks, a
/// final usage chunk with no `choices`, keepalives, and a stream that just
/// stops mid-JSON.
///
/// Also covers the provider deltas — DeepSeek vs OpenRouter differ in the
/// endpoint path, the reasoning dialect and how usage/cost is reported.
library;

import 'package:aa_bridge/agent/ai_client.dart';
import 'package:aa_bridge/agent/ai_models.dart';
import 'package:flutter_test/flutter_test.dart';

List<ChatEvent> feedAll(SseDecoder d, String body) => [
      for (final line in body.split('\n')) ...d.feedLine(line),
    ];

String chunk(Map<String, dynamic> delta, {String? finish}) =>
    'data: {"choices":[{"index":0,"delta":${_json(delta)}'
    '${finish == null ? '' : ',"finish_reason":"$finish"'}}]}';

String _json(Map<String, dynamic> m) {
  final parts = m.entries.map((e) {
    final v = e.value;
    return '"${e.key}":${v is String ? '"${v.replaceAll(r'\', r'\\').replaceAll('"', r'\"')}"' : v}';
  });
  return '{${parts.join(',')}}';
}

void main() {
  group('SseDecoder', () {
    test('assembles plain content', () {
      final d = SseDecoder();
      final events = feedAll(
          d,
          '${chunk({'content': 'Hello'})}\n'
          '${chunk({'content': ' world'}, finish: 'stop')}\n'
          'data: [DONE]');
      expect(events.whereType<ContentDelta>().map((e) => e.text),
          ['Hello', ' world']);
      final done = d.finish();
      expect(done.content, 'Hello world');
      expect(done.finishReason, 'stop');
      expect(done.wantsTools, isFalse);
    });

    test('ignores keepalives and blank lines', () {
      final d = SseDecoder();
      final events = feedAll(
          d,
          ': keepalive\n'
          '\n'
          '${chunk({'content': 'x'}, finish: 'stop')}');
      expect(events, hasLength(1));
      expect(d.finish().content, 'x');
    });

    test('reassembles tool-call arguments split across chunks', () {
      // The classic SSE bug: each fragment is valid text, none is valid JSON.
      const body = '''
data: {"choices":[{"index":0,"delta":{"tool_calls":[{"index":0,"id":"call_1","type":"function","function":{"name":"apply_patch","arguments":""}}]}}]}
data: {"choices":[{"index":0,"delta":{"tool_calls":[{"index":0,"function":{"arguments":"{\\"intent\\":"}}]}}]}
data: {"choices":[{"index":0,"delta":{"tool_calls":[{"index":0,"function":{"arguments":"\\"bump gain\\"}"}}]}}]}
data: {"choices":[{"index":0,"delta":{},"finish_reason":"tool_calls"}]}
data: [DONE]''';
      final d = SseDecoder();
      feedAll(d, body);
      final done = d.finish();
      expect(done.wantsTools, isTrue);
      expect(done.toolCalls, hasLength(1));
      final c = done.toolCalls.single;
      expect(c.id, 'call_1');
      expect(c.name, 'apply_patch');
      expect(c.argumentsJson, '{"intent":"bump gain"}');
    });

    test('keeps several tool calls apart by index', () {
      const body = '''
data: {"choices":[{"index":0,"delta":{"tool_calls":[{"index":0,"id":"a","function":{"name":"read_lines","arguments":"{\\"start\\":1}"}},{"index":1,"id":"b","function":{"name":"lint_script","arguments":"{}"}}]}}]}
data: {"choices":[{"index":0,"delta":{},"finish_reason":"tool_calls"}]}''';
      final d = SseDecoder();
      feedAll(d, body);
      final calls = d.finish().toolCalls;
      expect(calls.map((c) => c.name), ['read_lines', 'lint_script']);
      expect(calls.first.argumentsJson, '{"start":1}');
    });

    test('handles the usage-only final chunk (empty choices)', () {
      const body = '''
data: {"choices":[{"index":0,"delta":{"content":"hi"},"finish_reason":"stop"}]}
data: {"choices":[],"usage":{"prompt_tokens":1200,"completion_tokens":40,"prompt_cache_hit_tokens":1024,"prompt_cache_miss_tokens":176}}
data: [DONE]''';
      final d = SseDecoder();
      expect(() => feedAll(d, body), returnsNormally);
      final done = d.finish();
      expect(done.usage.promptTokens, 1200);
      expect(done.usage.cacheHitTokens, 1024);
      expect(done.usage.cacheRatio, closeTo(0.853, 0.01));
    });

    test('[DONE] without a usage chunk still completes', () {
      final d = SseDecoder();
      feedAll(d, '${chunk({'content': 'ok'}, finish: 'stop')}\ndata: [DONE]');
      final done = d.finish();
      expect(done.usage.promptTokens, 0);
      expect(done.content, 'ok');
    });

    test('streams reasoning separately from content', () {
      final d = SseDecoder();
      final events = feedAll(
          d,
          '${chunk({'reasoning_content': 'let me check'})}\n'
          '${chunk({'content': 'answer'}, finish: 'stop')}');
      expect(events.whereType<ReasoningDelta>().single.text, 'let me check');
      // Reasoning must not leak into the message echoed back to the API.
      expect(d.finish().content, 'answer');
      expect(d.finish().message.content, 'answer');
    });

    test('a malformed chunk is skipped, not fatal', () {
      final d = SseDecoder();
      feedAll(
          d,
          'data: {not json\n'
          '${chunk({'content': 'still here'}, finish: 'stop')}');
      expect(d.finish().content, 'still here');
    });

    test('truncation mid-stream leaves a usable partial turn', () {
      // Server died after one delta: no finish_reason, no [DONE].
      final d = SseDecoder();
      feedAll(d, '${chunk({'content': 'half'})}\ndata: {"choices":[{"index"');
      final done = d.finish();
      expect(done.content, 'half');
      expect(done.finishReason, 'stop');
    });

    test('an empty stream is a protocol error', () {
      expect(() => SseDecoder().finish(), throwsA(isA<DsBadResponse>()));
    });

    test('infers tool_calls when finish_reason is missing', () {
      const body =
          'data: {"choices":[{"index":0,"delta":{"tool_calls":[{"index":0,"id":"z","function":{"name":"finish","arguments":"{}"}}]}}]}';
      final d = SseDecoder();
      feedAll(d, body);
      expect(d.finish().finishReason, 'tool_calls');
    });
  });

  group('error mapping', () {
    test('maps the statuses that matter', () {
      expect(errorForStatus(401, 'bad key'), isA<DsAuthError>());
      expect(errorForStatus(402, 'no balance'), isA<DsInsufficientBalance>());
      expect(errorForStatus(429, 'slow down'), isA<DsRateLimited>());
      expect(errorForStatus(503, 'oops'), isA<DsServerError>());
      expect(errorForStatus(418, 'teapot'), isA<DsBadResponse>());
    });

    test('only transient failures are retryable', () {
      expect(errorForStatus(401, '').retryable, isFalse);
      expect(errorForStatus(402, '').retryable, isFalse);
      expect(errorForStatus(429, '').retryable, isTrue);
      expect(errorForStatus(500, '').retryable, isTrue);
      expect(const DsNetworkError().retryable, isTrue);
    });

    test('honours Retry-After', () {
      final e = errorForStatus(429, '', retryAfter: '7') as DsRateLimited;
      expect(e.retryAfter, const Duration(seconds: 7));
      expect(backoffFor(0, retryAfter: e.retryAfter),
          const Duration(seconds: 7));
    });

    test('backoff grows and stays positive', () {
      final a = backoffFor(0).inMilliseconds;
      final b = backoffFor(1).inMilliseconds;
      final c = backoffFor(2).inMilliseconds;
      expect(a, greaterThan(0));
      expect(b, greaterThan(a));
      expect(c, greaterThan(b));
      expect(backoffFor(3, jitter: -1).inMilliseconds, greaterThan(0));
    });

    test('a long error body is truncated in the detail', () {
      final e = errorForStatus(500, 'x' * 1000);
      expect(e.detail!.length, lessThan(320));
    });
  });

  group('provider differences', () {
    AiClient client(AiProvider p, {bool strict = true, String? baseUrl}) =>
        AiClient(
            apiKey: 'k', provider: p, strict: strict, baseUrl: baseUrl);

    test('DeepSeek puts strict schemas behind /beta', () {
      expect(client(AiProvider.deepseek).endpoint.toString(),
          'https://api.deepseek.com/beta/chat/completions');
      expect(client(AiProvider.deepseek, strict: false).endpoint.toString(),
          'https://api.deepseek.com/chat/completions');
    });

    test('OpenRouter keeps one path regardless of strict', () {
      expect(client(AiProvider.openrouter).endpoint.toString(),
          'https://openrouter.ai/api/v1/chat/completions');
      expect(client(AiProvider.openrouter, strict: false).endpoint.toString(),
          'https://openrouter.ai/api/v1/chat/completions');
    });

    test('a custom base URL is used verbatim, trailing slash and all', () {
      expect(
          client(AiProvider.custom, baseUrl: 'https://proxy.local/v1/')
              .endpoint
              .toString(),
          'https://proxy.local/v1/chat/completions');
    });

    test('a plaintext endpoint is refused — the key must not go in clear', () {
      expect(() => client(AiProvider.custom, baseUrl: 'http://proxy.local/v1').endpoint,
          throwsA(isA<DsBadResponse>()));
    });

    test('reasoning uses each provider\'s own dialect', () {
      Map<String, dynamic> wire(AiProvider p, bool think) => ChatRequest(
            provider: p,
            model: 'm',
            messages: const [ChatMessage.user('hi')],
            thinking: think,
          ).toWire();

      expect((wire(AiProvider.deepseek, true)['thinking'] as Map)['type'],
          'enabled');
      expect((wire(AiProvider.deepseek, false)['thinking'] as Map)['type'],
          'disabled');
      expect((wire(AiProvider.openrouter, true)['reasoning'] as Map)['enabled'],
          isTrue);
      expect(wire(AiProvider.openrouter, true).containsKey('thinking'), isFalse);
      // An unknown endpoint gets neither dialect.
      expect(wire(AiProvider.custom, true).containsKey('reasoning'), isFalse);
      expect(wire(AiProvider.custom, true).containsKey('thinking'), isFalse);
    });

    test('model ids are namespaced on OpenRouter only', () {
      expect(AiProvider.deepseek.defaultModel, 'deepseek-v4-flash');
      expect(AiProvider.openrouter.defaultModel, 'deepseek/deepseek-v4-flash');
    });

    test('OpenRouter cache accounting is read from prompt_tokens_details', () {
      final u = ChatUsage.fromJson(const {
        'prompt_tokens': 1000,
        'completion_tokens': 50,
        'prompt_tokens_details': {'cached_tokens': 800},
        'cost': 0.00123,
      });
      expect(u.cacheHitTokens, 800);
      expect(u.cacheMissTokens, 200);
      expect(u.reportedCostUsd, 0.00123);
    });

    test('a provider-reported cost wins over the local price table', () {
      const u = ChatUsage(
          promptTokens: 1000, completionTokens: 100, reportedCostUsd: 0.5);
      expect(estimateCost('deepseek-v4-flash', u), 0.5);
    });
  });

  group('request shape', () {
    test('carries stream + usage + serial tool calls', () {
      final w = ChatRequest(
        model: kDefaultModel,
        messages: const [ChatMessage.user('hi')],
        tools: [
          {
            'type': 'function',
            'function': {'name': 'lint_script'}
          }
        ],
      ).toWire();
      expect(w['stream'], isTrue);
      expect((w['stream_options'] as Map)['include_usage'], isTrue);
      expect(w['parallel_tool_calls'], isFalse);
      expect(w['tool_choice'], 'auto');
      // Default provider is OpenRouter, so reasoning is off in its dialect.
      expect((w['reasoning'] as Map)['enabled'], isFalse);
    });

    test('omits tool fields when there are no tools', () {
      final w = ChatRequest(
              model: kDefaultModel, messages: const [ChatMessage.user('hi')])
          .toWire();
      expect(w.containsKey('tools'), isFalse);
      expect(w.containsKey('parallel_tool_calls'), isFalse);
    });

    test('thinking mode carries the effort', () {
      final w = ChatRequest(
        model: kStrongModel,
        messages: const [ChatMessage.user('hi')],
        thinking: true,
        reasoningEffort: 'max',
      ).toWire();
      expect((w['reasoning'] as Map)['effort'], 'max');
      expect(
          ChatRequest(
            provider: AiProvider.deepseek,
            model: 'deepseek-v4-pro',
            messages: const [ChatMessage.user('hi')],
            thinking: true,
            reasoningEffort: 'max',
          ).toWire()['thinking'],
          containsPair('reasoning_effort', 'max'));
    });

    test('tool results serialise with their call id', () {
      final m = const ChatMessage.tool('call_9', '{"ok":true}').toWire();
      expect(m['role'], 'tool');
      expect(m['tool_call_id'], 'call_9');
      expect(m['content'], '{"ok":true}');
    });
  });

  group('pricing', () {
    test('costs a cached-heavy turn realistically', () {
      const u = ChatUsage(
          promptTokens: 20000,
          completionTokens: 500,
          cacheHitTokens: 18000,
          cacheMissTokens: 2000);
      final c = estimateCost(kDefaultModel, u)!;
      expect(c, greaterThan(0));
      expect(c, lessThan(0.01)); // a step should cost fractions of a cent
    });

    test('an unknown model has no price rather than a wrong one', () {
      expect(estimateCost('some-future-model', const ChatUsage()), isNull);
    });
  });
}
