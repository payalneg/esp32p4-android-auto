/// Wire types for the chat API plus the token accounting the UI shows.
///
/// The protocol is the OpenAI chat-completions shape, which both supported
/// providers speak: DeepSeek directly, or OpenRouter as a front for DeepSeek
/// (and anything else it hosts). The differences between them are small and
/// live in [AiProvider] rather than being sprinkled through the client.
library;

/// One tool invocation requested by the model.
class ToolCall {
  final String id;
  final String name;

  /// Raw JSON string — kept unparsed here because it arrives in fragments
  /// across SSE chunks and is only decoded once complete.
  final String argumentsJson;

  const ToolCall(
      {required this.id, required this.name, required this.argumentsJson});

  Map<String, dynamic> toWire() => {
        'id': id,
        'type': 'function',
        'function': {'name': name, 'arguments': argumentsJson},
      };

  Map<String, dynamic> toJson() =>
      {'id': id, 'name': name, 'arguments': argumentsJson};

  factory ToolCall.fromJson(Map<String, dynamic> m) => ToolCall(
        id: m['id'] as String? ?? '',
        name: m['name'] as String? ?? '',
        argumentsJson: m['arguments'] as String? ?? '{}',
      );
}

enum ChatRole { system, user, assistant, tool }

class ChatMessage {
  final ChatRole role;
  final String? content;
  final List<ToolCall> toolCalls;

  /// Set on [ChatRole.tool] messages: which call this answers.
  final String? toolCallId;

  const ChatMessage({
    required this.role,
    this.content,
    this.toolCalls = const [],
    this.toolCallId,
  });

  const ChatMessage.system(String text)
      : role = ChatRole.system,
        content = text,
        toolCalls = const [],
        toolCallId = null;

  const ChatMessage.user(String text)
      : role = ChatRole.user,
        content = text,
        toolCalls = const [],
        toolCallId = null;

  const ChatMessage.tool(String callId, String resultJson)
      : role = ChatRole.tool,
        content = resultJson,
        toolCalls = const [],
        toolCallId = callId;

  /// Assistant turns carry `content` + `tool_calls` only — `reasoning_content`
  /// is deliberately NOT echoed back (the API rejects it and it would bloat
  /// every subsequent request).
  Map<String, dynamic> toWire() => {
        'role': role.name,
        if (content != null) 'content': content,
        if (toolCalls.isNotEmpty)
          'tool_calls': [for (final c in toolCalls) c.toWire()],
        if (toolCallId != null) 'tool_call_id': toolCallId,
      };

  Map<String, dynamic> toJson() => {
        'role': role.name,
        if (content != null) 'content': content,
        if (toolCalls.isNotEmpty)
          'tool_calls': [for (final c in toolCalls) c.toJson()],
        if (toolCallId != null) 'tool_call_id': toolCallId,
      };

  factory ChatMessage.fromJson(Map<String, dynamic> m) => ChatMessage(
        role: ChatRole.values.firstWhere((e) => e.name == m['role'],
            orElse: () => ChatRole.user),
        content: m['content'] as String?,
        toolCalls: [
          for (final c in (m['tool_calls'] as List?) ?? const [])
            ToolCall.fromJson((c as Map).cast<String, dynamic>())
        ],
        toolCallId: m['tool_call_id'] as String?,
      );
}

/// Token usage of one request. Both providers cache prompt prefixes and
/// report hits separately — a high hit ratio is the signal that the stable
/// system prefix is intact (see agent_prompt.dart). They spell it differently:
/// DeepSeek uses `prompt_cache_hit_tokens`, OpenRouter uses
/// `prompt_tokens_details.cached_tokens`.
class ChatUsage {
  final int promptTokens;
  final int completionTokens;
  final int cacheHitTokens;
  final int cacheMissTokens;

  /// What the provider says the call cost, in USD. OpenRouter reports this
  /// directly and it is authoritative — preferred over any local price table.
  final double? reportedCostUsd;

  const ChatUsage({
    this.promptTokens = 0,
    this.completionTokens = 0,
    this.cacheHitTokens = 0,
    this.cacheMissTokens = 0,
    this.reportedCostUsd,
  });

  double get cacheRatio =>
      promptTokens > 0 ? cacheHitTokens / promptTokens : 0;

  ChatUsage operator +(ChatUsage o) => ChatUsage(
        promptTokens: promptTokens + o.promptTokens,
        completionTokens: completionTokens + o.completionTokens,
        cacheHitTokens: cacheHitTokens + o.cacheHitTokens,
        cacheMissTokens: cacheMissTokens + o.cacheMissTokens,
        reportedCostUsd: (reportedCostUsd == null && o.reportedCostUsd == null)
            ? null
            : (reportedCostUsd ?? 0) + (o.reportedCostUsd ?? 0),
      );

  factory ChatUsage.fromJson(Map<String, dynamic> m) {
    final details = m['prompt_tokens_details'];
    final cachedOr = details is Map
        ? (details['cached_tokens'] as num?)?.toInt()
        : null;
    final prompt = (m['prompt_tokens'] as num?)?.toInt() ?? 0;
    final hit = (m['prompt_cache_hit_tokens'] as num?)?.toInt() ?? cachedOr ?? 0;
    final miss = (m['prompt_cache_miss_tokens'] as num?)?.toInt() ??
        (prompt - hit).clamp(0, prompt);
    return ChatUsage(
      promptTokens: prompt,
      completionTokens: (m['completion_tokens'] as num?)?.toInt() ?? 0,
      cacheHitTokens: hit,
      cacheMissTokens: miss,
      reportedCostUsd: (m['cost'] as num?)?.toDouble(),
    );
  }

  Map<String, dynamic> toJson() => {
        'prompt_tokens': promptTokens,
        'completion_tokens': completionTokens,
        'prompt_cache_hit_tokens': cacheHitTokens,
        'prompt_cache_miss_tokens': cacheMissTokens,
        if (reportedCostUsd != null) 'cost': reportedCostUsd,
      };
}

/// Published price per 1M tokens. [updated] is shown in the settings screen so
/// a stale table is visible rather than silently wrong; an unknown model id
/// shows a dash instead of a made-up number.
class ModelPricing {
  final double inHitPerM;
  final double inMissPerM;
  final double outPerM;
  final String updated;

  const ModelPricing({
    required this.inHitPerM,
    required this.inMissPerM,
    required this.outPerM,
    required this.updated,
  });

  double costOf(ChatUsage u) =>
      u.cacheHitTokens / 1e6 * inHitPerM +
      u.cacheMissTokens / 1e6 * inMissPerM +
      u.completionTokens / 1e6 * outPerM;
}

/// Fallback price table, used only when the provider doesn't report a cost
/// (DeepSeek direct). Prices as published on the [updated] date.
const kPricing = <String, ModelPricing>{
  'deepseek-v4-flash': ModelPricing(
      inHitPerM: 0.0028, inMissPerM: 0.14, outPerM: 0.28, updated: '2026-07-30'),
  'deepseek-v4-pro': ModelPricing(
      inHitPerM: 0.003625,
      inMissPerM: 0.435,
      outPerM: 0.87,
      updated: '2026-07-30'),
  'deepseek/deepseek-v4-flash': ModelPricing(
      inHitPerM: 0.09, inMissPerM: 0.09, outPerM: 0.18, updated: '2026-07-30'),
  'deepseek/deepseek-v4-pro': ModelPricing(
      inHitPerM: 0.435, inMissPerM: 0.435, outPerM: 0.87, updated: '2026-07-30'),
};

/// Where the chat API lives. The wire protocol is the same OpenAI shape for
/// all of them; these are the deltas.
enum AiProvider {
  /// api.deepseek.com. Strict function schemas require the `/beta` path;
  /// thinking mode is `thinking: {type, reasoning_effort}`.
  deepseek,

  /// openrouter.ai — a front for DeepSeek and everything else. Model ids are
  /// namespaced (`deepseek/deepseek-v4-flash`), cost is reported per call, and
  /// reasoning is `reasoning: {enabled, effort}`.
  openrouter,

  /// Anything else OpenAI-compatible (a local llama.cpp, a proxy). Only the
  /// common subset is sent.
  custom,
}

extension AiProviderInfo on AiProvider {
  String get label => switch (this) {
        AiProvider.deepseek => 'DeepSeek',
        AiProvider.openrouter => 'OpenRouter',
        AiProvider.custom => 'Custom',
      };

  String get defaultBaseUrl => switch (this) {
        AiProvider.deepseek => 'https://api.deepseek.com',
        AiProvider.openrouter => 'https://openrouter.ai/api/v1',
        AiProvider.custom => '',
      };

  /// Cheap default: the agent loop is dominated by tool round-trips, not by
  /// depth of reasoning, and flash is ~3-5x cheaper than pro.
  String get defaultModel => switch (this) {
        AiProvider.deepseek => 'deepseek-v4-flash',
        AiProvider.openrouter => 'deepseek/deepseek-v4-flash',
        AiProvider.custom => '',
      };

  String get strongModel => switch (this) {
        AiProvider.deepseek => 'deepseek-v4-pro',
        AiProvider.openrouter => 'deepseek/deepseek-v4-pro',
        AiProvider.custom => '',
      };

  /// Only DeepSeek moves strict-schema support to a different base path.
  bool get strictUsesBetaPath => this == AiProvider.deepseek;

  String get keyUrl => switch (this) {
        AiProvider.deepseek => 'platform.deepseek.com',
        AiProvider.openrouter => 'openrouter.ai/keys',
        AiProvider.custom => '',
      };
}

const kDefaultProvider = AiProvider.openrouter;
String get kDefaultModel => kDefaultProvider.defaultModel;
String get kStrongModel => kDefaultProvider.strongModel;
String get kDefaultBaseUrl => kDefaultProvider.defaultBaseUrl;

/// Best available cost for a turn: whatever the provider charged if it said,
/// else the local table. `null` means "unknown" — render a dash, never a
/// guess.
double? estimateCost(String model, ChatUsage usage) =>
    usage.reportedCostUsd ?? kPricing[model]?.costOf(usage);
