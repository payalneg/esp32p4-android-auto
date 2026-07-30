/// Bounds on one agent session.
///
/// Two of these are safety limits rather than cost limits: [maxFlashes] caps
/// how many times a runaway loop can rewrite the VESC's flash, and [maxWall]
/// stops a session that has quietly stalled. Hitting a limit does not cut the
/// conversation dead — the loop forces one final `finish` turn so the user
/// gets a summary of where things stand.
library;

enum BudgetVerdict { ok, exhausted }

class AgentUsageCounters {
  int steps = 0;
  int toolCalls = 0;
  int flashes = 0;
  double spentUsd = 0;
  final Stopwatch clock = Stopwatch();
}

class AgentBudget {
  const AgentBudget({
    this.maxSteps = 24,
    this.maxToolCalls = 60,
    this.maxFlashes = 6,
    this.maxWall = const Duration(minutes: 12),
    this.maxSpendUsd = 0.50,
  });

  final int maxSteps;
  final int maxToolCalls;
  final int maxFlashes;
  final Duration maxWall;

  /// 0 disables the spend cap.
  final double maxSpendUsd;

  /// i18n key of the limit that was hit, or null while there's room left.
  String? exhaustedKey(AgentUsageCounters u) {
    if (u.steps >= maxSteps) return 'agent.budget.steps';
    if (u.toolCalls >= maxToolCalls) return 'agent.budget.tools';
    if (u.clock.elapsed >= maxWall) return 'agent.budget.time';
    if (maxSpendUsd > 0 && u.spentUsd >= maxSpendUsd) return 'agent.budget.spend';
    return null;
  }

  BudgetVerdict check(AgentUsageCounters u) =>
      exhaustedKey(u) == null ? BudgetVerdict.ok : BudgetVerdict.exhausted;

  /// Flashing is capped separately: it is checked inside the tool so the model
  /// gets a normal tool error it can reason about, rather than the session
  /// ending under it.
  bool canFlash(AgentUsageCounters u) => u.flashes < maxFlashes;

  AgentBudget copyWith({int? maxSteps, int? maxFlashes, double? maxSpendUsd}) =>
      AgentBudget(
        maxSteps: maxSteps ?? this.maxSteps,
        maxToolCalls: maxToolCalls,
        maxFlashes: maxFlashes ?? this.maxFlashes,
        maxWall: maxWall,
        maxSpendUsd: maxSpendUsd ?? this.maxSpendUsd,
      );
}
