/// Streaming client for an OpenAI-compatible chat API — DeepSeek directly, or
/// OpenRouter fronting it (see [AiProvider]).
///
/// Uses `dart:io HttpClient` directly — the same thing the firmware OTA
/// uploader does (firmware_updater.dart) — because `package:http` would be a
/// new dependency and still wouldn't frame SSE for us. The framing is ~40
/// lines and lives in [SseDecoder], which is pure and therefore testable
/// against recorded fixtures with no network.
///
/// The two SSE traps this handles explicitly:
///   * tool-call arguments arrive as fragments spread over many chunks and
///     must be concatenated per index before being parsed;
///   * the final usage chunk has an empty `choices` list, so anything that
///     assumes `choices[0]` exists crashes on the last message.
library;

import 'dart:async';
import 'dart:convert';
import 'dart:io';
import 'dart:math' as math;

import 'package:flutter/foundation.dart' show visibleForTesting;

import 'ai_models.dart';
import 'cancel_token.dart';

// ---------------------------------------------------------------------------
// Errors
// ---------------------------------------------------------------------------

sealed class DeepSeekError implements Exception {
  const DeepSeekError(this.detail);
  final String? detail;

  /// i18n key the UI localizes.
  String get i18nKey;

  /// Whether re-issuing the same request could plausibly succeed.
  bool get retryable => false;

  @override
  String toString() => '$runtimeType(${detail ?? ''})';
}

class DsAuthError extends DeepSeekError {
  const DsAuthError([super.detail]);
  @override
  String get i18nKey => 'agent.err.auth';
}

class DsInsufficientBalance extends DeepSeekError {
  const DsInsufficientBalance([super.detail]);
  @override
  String get i18nKey => 'agent.err.balance';
}

class DsRateLimited extends DeepSeekError {
  const DsRateLimited({this.retryAfter, String? detail}) : super(detail);
  final Duration? retryAfter;
  @override
  String get i18nKey => 'agent.err.ratelimit';
  @override
  bool get retryable => true;
}

class DsServerError extends DeepSeekError {
  const DsServerError(this.status, [super.detail]);
  final int status;
  @override
  String get i18nKey => 'agent.err.server';
  @override
  bool get retryable => true;
}

class DsNetworkError extends DeepSeekError {
  const DsNetworkError([super.detail]);
  @override
  String get i18nKey => 'agent.err.network';
  @override
  bool get retryable => true;
}

class DsBadResponse extends DeepSeekError {
  const DsBadResponse([super.detail]);
  @override
  String get i18nKey => 'agent.err.protocol';
}

class DsNoKey extends DeepSeekError {
  const DsNoKey() : super(null);
  @override
  String get i18nKey => 'agent.err.nokey';
}

/// Map an HTTP status onto the right error type. Split out so the mapping is
/// testable without a socket.
DeepSeekError errorForStatus(int status, String body, {String? retryAfter}) {
  final detail = body.length > 300 ? '${body.substring(0, 300)}…' : body;
  return switch (status) {
    401 || 403 => DsAuthError(detail),
    402 => DsInsufficientBalance(detail),
    429 => DsRateLimited(
        retryAfter: _parseRetryAfter(retryAfter), detail: detail),
    >= 500 => DsServerError(status, detail),
    _ => DsBadResponse('HTTP $status: $detail'),
  };
}

Duration? _parseRetryAfter(String? h) {
  if (h == null) return null;
  final secs = int.tryParse(h.trim());
  return secs == null ? null : Duration(seconds: secs.clamp(0, 120));
}

/// Exponential backoff with ±30% jitter, honouring a server-sent Retry-After.
Duration backoffFor(int attempt, {Duration? retryAfter, double jitter = 0}) {
  if (retryAfter != null) return retryAfter;
  final base = 500 * math.pow(2, attempt).toInt();
  final spread = (base * 0.3 * jitter).round();
  return Duration(milliseconds: math.max(100, base + spread));
}

// ---------------------------------------------------------------------------
// Stream events
// ---------------------------------------------------------------------------

sealed class ChatEvent {
  const ChatEvent();
}

class ContentDelta extends ChatEvent {
  const ContentDelta(this.text);
  final String text;
}

/// Thinking-mode output. Streamed to a collapsed tile and NEVER fed back into
/// the next request.
class ReasoningDelta extends ChatEvent {
  const ReasoningDelta(this.text);
  final String text;
}

class ChatCompleted extends ChatEvent {
  const ChatCompleted({
    required this.finishReason,
    required this.content,
    required this.toolCalls,
    required this.usage,
  });
  final String finishReason;
  final String content;
  final List<ToolCall> toolCalls;
  final ChatUsage usage;

  bool get wantsTools =>
      finishReason == 'tool_calls' || toolCalls.isNotEmpty;

  /// The assistant turn to append to the transcript.
  ChatMessage get message => ChatMessage(
        role: ChatRole.assistant,
        content: content.isEmpty ? null : content,
        toolCalls: toolCalls,
      );
}

// ---------------------------------------------------------------------------
// SSE decoding
// ---------------------------------------------------------------------------

/// Turns SSE lines into [ChatEvent]s and accumulates the assistant turn.
///
/// Pure and stateful: feed it every line of the response body in order, then
/// call [finish]. Kept separate from the transport so the nasty cases —
/// fragmented tool arguments, a usage-only final chunk, `[DONE]` with no
/// usage, a stream that just stops — are all unit-testable.
class SseDecoder {
  final _content = StringBuffer();
  final _toolIds = <int, String>{};
  final _toolNames = <int, String>{};
  final _toolArgs = <int, StringBuffer>{};

  String? _finishReason;
  ChatUsage _usage = const ChatUsage();
  bool _sawAnyChunk = false;

  bool get sawAnyChunk => _sawAnyChunk;
  String get contentSoFar => _content.toString();

  /// Feed one raw line. Returns the events it produced (usually 0 or 1).
  List<ChatEvent> feedLine(String line) {
    final l = line.trimRight();
    if (l.isEmpty || l.startsWith(':')) return const []; // keepalive
    if (!l.startsWith('data:')) return const [];
    final payload = l.substring(5).trim();
    if (payload == '[DONE]') return const [];

    final Map<String, dynamic> obj;
    try {
      obj = jsonDecode(payload) as Map<String, dynamic>;
    } catch (_) {
      // A malformed chunk must not kill a stream that is otherwise fine.
      return const [];
    }
    _sawAnyChunk = true;

    final usage = obj['usage'];
    if (usage is Map) {
      _usage = ChatUsage.fromJson(usage.cast<String, dynamic>());
    }

    // The final usage chunk carries an empty choices list.
    final choices = obj['choices'];
    if (choices is! List || choices.isEmpty) return const [];
    final choice = (choices.first as Map).cast<String, dynamic>();

    final fr = choice['finish_reason'];
    if (fr is String && fr.isNotEmpty) _finishReason = fr;

    final delta = choice['delta'];
    if (delta is! Map) return const [];
    final d = delta.cast<String, dynamic>();

    final out = <ChatEvent>[];

    final reasoning = d['reasoning_content'];
    if (reasoning is String && reasoning.isNotEmpty) {
      out.add(ReasoningDelta(reasoning));
    }

    final content = d['content'];
    if (content is String && content.isNotEmpty) {
      _content.write(content);
      out.add(ContentDelta(content));
    }

    final calls = d['tool_calls'];
    if (calls is List) {
      for (final raw in calls) {
        if (raw is! Map) continue;
        final c = raw.cast<String, dynamic>();
        final idx = (c['index'] as num?)?.toInt() ?? 0;
        final id = c['id'];
        if (id is String && id.isNotEmpty) _toolIds[idx] = id;
        final fn = c['function'];
        if (fn is Map) {
          final name = fn['name'];
          if (name is String && name.isNotEmpty) _toolNames[idx] = name;
          final args = fn['arguments'];
          if (args is String && args.isNotEmpty) {
            (_toolArgs[idx] ??= StringBuffer()).write(args);
          }
        }
      }
    }
    return out;
  }

  /// Close out the turn. Throws [DsBadResponse] if nothing usable arrived.
  ChatCompleted finish() {
    if (!_sawAnyChunk) {
      throw const DsBadResponse('empty stream');
    }
    final calls = <ToolCall>[];
    final indices = _toolNames.keys.toList()..sort();
    for (final i in indices) {
      calls.add(ToolCall(
        id: _toolIds[i] ?? 'call_$i',
        name: _toolNames[i]!,
        argumentsJson: _toolArgs[i]?.toString() ?? '{}',
      ));
    }
    return ChatCompleted(
      finishReason: _finishReason ?? (calls.isEmpty ? 'stop' : 'tool_calls'),
      content: _content.toString(),
      toolCalls: calls,
      usage: _usage,
    );
  }
}

// ---------------------------------------------------------------------------
// Client
// ---------------------------------------------------------------------------

class ChatRequest {
  const ChatRequest({
    required this.model,
    required this.messages,
    this.tools = const [],
    this.temperature = 0,
    this.thinking = false,
    this.reasoningEffort = 'high',
    this.toolChoice,
    this.maxTokens,
    this.provider = kDefaultProvider,
  });

  final AiProvider provider;
  final String model;
  final List<ChatMessage> messages;

  /// JSON-Schema tool definitions, already in OpenAI `tools` shape.
  final List<Map<String, dynamic>> tools;
  final double temperature;
  final bool thinking;
  final String reasoningEffort;

  /// `auto` by default; set to `{"type":"function","function":{"name":…}}` to
  /// force a specific tool (used to force `finish` when the budget runs out).
  final Object? toolChoice;
  final int? maxTokens;

  Map<String, dynamic> toWire() => {
        'model': model,
        'messages': [for (final m in messages) m.toWire()],
        'stream': true,
        'stream_options': {'include_usage': true},
        'temperature': temperature,
        if (maxTokens != null) 'max_tokens': maxTokens,
        if (tools.isNotEmpty) ...{
          'tools': tools,
          'tool_choice': toolChoice ?? 'auto',
          // Concurrent tool calls would interleave working-copy mutations and
          // confirmation prompts; the loop enforces this client-side too.
          'parallel_tool_calls': false,
        },
        // Reasoning is the one place the providers genuinely differ. Sending
        // the wrong dialect is either ignored or rejected, so pick by
        // provider and send nothing at all for an unknown endpoint.
        ...switch (provider) {
          AiProvider.deepseek => {
              'thinking': thinking
                  ? {'type': 'enabled', 'reasoning_effort': reasoningEffort}
                  : {'type': 'disabled'},
            },
          AiProvider.openrouter => {
              'reasoning': thinking
                  ? {'enabled': true, 'effort': reasoningEffort}
                  : {'exclude': true, 'enabled': false},
            },
          AiProvider.custom => const <String, dynamic>{},
        },
      };
}

class AiClient {
  AiClient({
    required this.apiKey,
    this.provider = kDefaultProvider,
    String? baseUrl,
    this.strict = true,
    this.appTitle = 'VESC Display Tool',
    this.connectTimeout = const Duration(seconds: 20),
    this.idleTimeout = const Duration(seconds: 120),
    @visibleForTesting HttpClient Function()? httpClientFactory,
  })  : baseUrl = (baseUrl == null || baseUrl.trim().isEmpty)
            ? provider.defaultBaseUrl
            : baseUrl.trim(),
        _newClient = httpClientFactory ?? HttpClient.new;

  final String apiKey;
  final AiProvider provider;
  final String baseUrl;

  /// Ask the API to guarantee schema conformance for function arguments —
  /// worth a lot for `apply_patch`. On DeepSeek that means the `/beta` path;
  /// elsewhere it is a per-function flag set by the tool registry.
  final bool strict;

  /// Sent as `X-Title` so OpenRouter can attribute the traffic.
  final String appTitle;
  final Duration connectTimeout;
  final Duration idleTimeout;
  final HttpClient Function() _newClient;

  @visibleForTesting
  Uri get endpoint {
    var base = baseUrl;
    while (base.endsWith('/')) {
      base = base.substring(0, base.length - 1);
    }
    final beta = strict && provider.strictUsesBetaPath ? '/beta' : '';
    final uri = Uri.parse('$base$beta/chat/completions');
    // The manifest sets usesCleartextTraffic for the SoftAP OTA, so nothing
    // else would stop a mistyped http:// endpoint sending the key in clear.
    if (uri.scheme != 'https') {
      throw const DsBadResponse('endpoint must be https');
    }
    return uri;
  }

  /// Stream one assistant turn.
  ///
  /// Retries only failures that happen BEFORE any content was emitted, and
  /// only for 429/5xx/network. Once deltas have reached the UI a retry would
  /// duplicate them, so mid-stream failures propagate and the caller decides.
  Stream<ChatEvent> stream(ChatRequest req,
      {CancelToken? cancel, int maxAttempts = 3}) async* {
    if (apiKey.trim().isEmpty) throw const DsNoKey();

    for (var attempt = 0;; attempt++) {
      cancel?.throwIfCancelled();
      var emitted = false;
      try {
        await for (final e in _once(req, cancel)) {
          emitted = true;
          yield e;
        }
        return;
      } on DeepSeekError catch (e) {
        final canRetry =
            e.retryable && !emitted && attempt < maxAttempts - 1;
        if (!canRetry) rethrow;
        final wait = backoffFor(
          attempt,
          retryAfter: e is DsRateLimited ? e.retryAfter : null,
          jitter: _jitter(attempt),
        );
        await Future.any([
          Future<void>.delayed(wait),
          if (cancel != null) cancel.whenCancelled,
        ]);
        cancel?.throwIfCancelled();
      }
    }
  }

  /// Deterministic-enough spread without pulling in Random: attempts differ,
  /// which is all the jitter is for.
  double _jitter(int attempt) => ((attempt * 37) % 21 - 10) / 10.0;

  Stream<ChatEvent> _once(ChatRequest req, CancelToken? cancel) async* {
    final client = _newClient()..connectionTimeout = connectTimeout;
    void kill() => client.close(force: true);
    cancel?.onCancel(kill);

    final decoder = SseDecoder();
    try {
      final HttpClientRequest request;
      try {
        request = await client.postUrl(endpoint);
      } on SocketException catch (e) {
        throw DsNetworkError(e.message);
      } on HttpException catch (e) {
        throw DsNetworkError(e.message);
      }

      request.headers
        ..contentType = ContentType('application', 'json', charset: 'utf-8')
        ..set(HttpHeaders.authorizationHeader, 'Bearer $apiKey')
        ..set(HttpHeaders.acceptHeader, 'text/event-stream');
      if (provider == AiProvider.openrouter) {
        // Attribution headers: optional, but they are how a key's traffic is
        // identified on the OpenRouter dashboard.
        request.headers
          ..set('HTTP-Referer', 'https://github.com/payalneg/esp32p4-android-auto')
          ..set('X-Title', appTitle);
      }
      request.add(utf8.encode(jsonEncode(req.toWire())));

      final HttpClientResponse resp;
      try {
        resp = await request.close();
      } on SocketException catch (e) {
        throw DsNetworkError(e.message);
      } on HttpException catch (e) {
        throw DsNetworkError(e.message);
      }

      if (resp.statusCode != 200) {
        final body = await resp.transform(utf8.decoder).join();
        throw errorForStatus(resp.statusCode, body,
            retryAfter: resp.headers.value('retry-after'));
      }

      final lines = resp
          .transform(utf8.decoder)
          .transform(const LineSplitter())
          .timeout(idleTimeout, onTimeout: (sink) {
        sink.addError(const DsNetworkError('stream stalled'));
        sink.close();
      });

      try {
        await for (final line in lines) {
          cancel?.throwIfCancelled();
          for (final e in decoder.feedLine(line)) {
            yield e;
          }
        }
      } on SocketException catch (e) {
        throw DsNetworkError(e.message);
      } on HttpException catch (e) {
        throw DsNetworkError(e.message);
      }

      yield decoder.finish();
    } finally {
      cancel?.removeHook(kill);
      client.close(force: true);
    }
  }
}
