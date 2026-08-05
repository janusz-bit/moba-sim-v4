#pragma once

#include <functional>
#include <stdexcept>
#include <string>
#include <vector>

#include "effects/effect_key.hpp"
#include "effects/lifetime.hpp"
#include "effects/stack_policy.hpp"
#include "sim/tick.hpp"
#include "stats/modifier.hpp"
#include "stats/stat_id.hpp"
#include "stats/stat_mask.hpp"
#include "stats/stat_table.hpp"

namespace moba_sim {

/// Thrown when an effect reads a stat it did not declare in `reads`, or writes
/// one it did not declare in `writes`.
///
/// This is what makes the declared masks trustworthy rather than decorative:
/// the ordering guarantees below are only sound if the declarations are
/// complete, so an undeclared access is a hard error, not a warning.
class UndeclaredStatAccess : public std::logic_error {
  public:
    UndeclaredStatAccess(const EffectKey& key, StatId stat, const char* access)
        : std::logic_error(std::string{"effect '"} + key.label() + "' " + access +
                           " undeclared stat " + std::string{stat_name(stat)}),
          stat_(stat) {}

    [[nodiscard]] StatId stat() const { return stat_; }

  private:
    StatId stat_;
};

/// Read-only access to stat values for an effect, restricted to the stats the
/// effect declared it reads.
///
/// Every value returned here is final: EffectSet orders effects so that
/// everything an effect declares it reads has already received all of its
/// contributions. The one deliberate exception is a stat the effect both reads
/// and writes, which reads as "the value so far" — that is the layering
/// semantics of "gain 10% of your total AD as bonus AD", and it is why this
/// system needs no convergence loop.
class StatView {
  public:
    StatView(const StatTable& table, const StatMask& reads, const EffectKey& key)
        : table_(&table), reads_(&reads), key_(&key) {}

    /// Value of `stat`. Throws UndeclaredStatAccess if `stat` is not in the
    /// effect's `reads` mask.
    [[nodiscard]] double operator()(StatId stat) const { return get(stat); }
    [[nodiscard]] double get(StatId stat) const {
        if (!reads_->contains(stat)) {
            throw UndeclaredStatAccess{*key_, stat, "read"};
        }
        return table_->compute(stat);
    }

  private:
    const StatTable* table_;
    const StatMask* reads_;
    const EffectKey* key_;
};

/// Where an effect puts its modifiers. Restricted to the stats the effect
/// declared it writes, and automatically labelled with the effect's key and
/// scaled by its current stack count.
class ModifierSink {
  public:
    ModifierSink(StatTable& table, const StatMask& writes, const EffectKey& key, StackCount stacks)
        : table_(&table), writes_(&writes), key_(&key), stacks_(stacks) {}

    /// Adds `value` to the Base bucket of `stat`, scaled by the stack count.
    void add_base(StatId stat, double value);
    /// Adds `value` to the Inc bucket of `stat`, scaled by the stack count.
    void add_inc(StatId stat, double value);
    /// Adds `value` to the More bucket of `stat`. N stacks compound:
    /// (1 + value)^N, matching "the effect applied N times".
    void add_more(StatId stat, double value);

    /// Adds a pre-built modifier, honouring the stack count. The modifier's
    /// own `source` is ignored in favour of the effect's key label.
    void add(const Modifier& mod);

    /// Adds `value` exactly once regardless of stacks. For effects whose
    /// stacks do not scale the contribution linearly (e.g. stacks gate a
    /// threshold rather than an amount).
    void add_unstacked(StatId stat, ModifierKind kind, double value);

    /// The number of stacks currently on the effect.
    [[nodiscard]] StackCount stacks() const { return stacks_; }

  private:
    void check(StatId stat) const;
    [[nodiscard]] std::string label() const;

    StatTable* table_;
    const StatMask* writes_;
    const EffectKey* key_;
    StackCount stacks_;
};

/// Everything an effect is allowed to know while contributing.
struct EffectContext {
    StatView stats; // declared reads only
    Tick now;       // current simulation time
    TickRate rate;  // for effects that think in seconds
    StackCount stacks = 1;
};

/// A stat-modifying effect: buff, debuff, aura, item passive.
///
/// The shape of this type is the whole design. An effect declares, as data:
/// its identity (`key`), what it depends on (`reads`), what it changes
/// (`writes`), how long it lives (`lifetime`) and how it combines with itself
/// (`policy`). Only the arithmetic is a callable.
///
/// That split is what buys, at no cost to the effect author:
///   * exact single-pass evaluation in dependency order — no fixed-point
///     iteration, no epsilon, no ConvergenceError, so results do not depend on
///     solver settings;
///   * cycles rejected when an effect is applied, as a modelling error with a
///     clear message, instead of a numerical failure in the hot path;
///   * refresh, extend, stack, dispel and "time remaining" handled once by the
///     framework instead of re-implemented in every effect;
///   * `contribute` called exactly once per evaluation, so it does not need to
///     be idempotent for correctness — and being pure, evaluation can be
///     repeated freely.
///
/// `contribute` must not have side effects: it may be called again whenever
/// stats are recomputed. Anything that happens once per step belongs in the
/// advance phase (EffectSet::advance).
struct Effect {
    EffectKey key{};
    StatMask reads{};  // stats contribute() may query — must be complete
    StatMask writes{}; // stats contribute() may modify — must be complete
    Lifetime lifetime = Permanent{};
    StackPolicy policy = StackPolicy::Refresh;

    /// Compared by ReplaceIfStronger; also a convenient place to keep the
    /// effect's headline number so `contribute` can stay generic.
    double magnitude = 0.0;

    /// Upper bound for StackPolicy::Stack. Ignored by other policies.
    StackCount max_stacks = 1;

    /// The arithmetic: read declared stats, push modifiers into declared ones.
    std::function<void(const EffectContext&, ModifierSink&)> contribute{};
};

/// Builds a plain "flat bonus" effect: no reads, one or more fixed modifiers.
/// Covers the majority of buffs, so most call sites need no lambda at all.
[[nodiscard]] Effect flat_effect(EffectKey key, std::vector<Modifier> mods,
                                 Lifetime lifetime = Permanent{});

} // namespace moba_sim
