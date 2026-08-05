#pragma once

#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include "effects/effect.hpp"
#include "sim/tick.hpp"
#include "stats/stat_table.hpp"

namespace moba_sim {

/// Thrown when applying an effect would make the dependency graph circular,
/// e.g. one effect converting AD into AP while another converts AP into AD.
///
/// This is raised at apply time, not during evaluation, because it is a
/// modelling error: the value such a pair should produce is not defined. Note
/// that an effect reading a stat it also writes is *not* a cycle — that is
/// ordinary layering ("gain 10% of your total AD as bonus AD") and reads the
/// value accumulated so far.
class EffectCycleError : public std::logic_error {
  public:
    explicit EffectCycleError(const std::string& detail)
        : std::logic_error("circular effect dependency: " + detail) {}
};

/// Identifies one live effect instance inside an EffectSet.
///
/// Deliberately distinct from EffectKey: the key says *what* an effect is and
/// drives stacking, while a handle refers to *this particular instance* and is
/// only used to remove one of several Independent instances. Handles are
/// meaningless outside the EffectSet that issued them and are never used to
/// decide whether two effects are "the same".
enum class EffectHandle : std::uint64_t {};

/// A live effect: the effect itself plus the runtime state the framework owns.
struct EffectInstance {
    EffectHandle handle{};
    Effect effect{};
    StackCount stacks = 1;
    Tick applied_at{};
};

/// The set of effects on one unit, and the thing that turns them into stats.
///
/// Two phases, deliberately split because they have opposite requirements:
///
///   * `advance(now)` — mutates: expires finished effects, retires spent
///     OneShots. Runs exactly once per simulation step.
///   * `contribute_all(table, now, rate)` — pure: evaluates every effect once,
///     in dependency order, into an already-seeded StatTable. Never removes
///     anything, so it can run as often as stats are needed.
///
/// Fusing those two into one function is what forces "evaluate repeatedly but
/// only honour removal on the last pass" hacks, and makes every side effect
/// fire an unpredictable number of times. Keeping them apart means
/// `contribute` is pure by construction.
class EffectSet {
  public:
    /// Adds `effect`, resolving collisions with any live effect sharing its key
    /// according to `effect.policy`. Returns a handle to the instance that
    /// ended up live, or nullopt if the policy dropped the application
    /// (IgnoreIfPresent, or ReplaceIfStronger losing to the incumbent).
    ///
    /// Throws EffectCycleError if the effect would close a dependency cycle;
    /// the set is left unchanged in that case.
    std::optional<EffectHandle> apply(Effect effect, Tick now);

    /// Removes every instance with `key`. Returns how many were removed.
    std::size_t remove(const EffectKey& key);

    /// Removes one specific instance. Returns true if it was there.
    bool remove(EffectHandle handle);

    /// Removes every effect whose key satisfies `pred` — the dispel/cleanse
    /// primitive. Returns how many were removed.
    std::size_t remove_if(const std::function<bool(const EffectKey&)>& pred);

    /// Removes everything.
    void clear();

    /// Mutating step: drops expired effects and effects whose OneShot has been
    /// spent. Returns the keys that went away, in removal order, so callers can
    /// react (log it, fire an "expired" event).
    ///
    /// Call once per simulation step, before reading stats for that step.
    std::vector<EffectKey> advance(Tick now);

    /// Pure step: runs every live effect once, in dependency order, adding its
    /// modifiers to `table`. `table` must already hold the unit's base stats
    /// and item modifiers — effects layer on top of those.
    void contribute_all(StatTable& table, Tick now, TickRate rate = kDefaultTickRate) const;

    /// The live instances, in application order (not evaluation order).
    [[nodiscard]] const std::vector<EffectInstance>& instances() const { return instances_; }

    /// Evaluation order as indices into instances(), for debugging and tests.
    [[nodiscard]] const std::vector<std::size_t>& evaluation_order() const;

    [[nodiscard]] bool empty() const { return instances_.empty(); }
    [[nodiscard]] std::size_t size() const { return instances_.size(); }

    /// The first live instance with `key`, or nullptr.
    [[nodiscard]] const EffectInstance* find(const EffectKey& key) const;

    /// Stacks on `key`, or 0 when it is not active.
    [[nodiscard]] StackCount stacks_of(const EffectKey& key) const;

    /// Remaining duration of `key` at `now`: nullopt when absent or when the
    /// lifetime has no knowable end. This answers "how long is my buff up for"
    /// — a question the framework can only answer because it owns the deadline.
    [[nodiscard]] std::optional<TickSpan> remaining_on(const EffectKey& key, Tick now) const;

    /// The union of every live effect's `writes` mask: the stats that would
    /// change if all effects vanished. Lets an owner invalidate precisely.
    [[nodiscard]] StatMask written_stats() const;

  private:
    std::vector<EffectInstance>::iterator find_mut(const EffectKey& key);
    /// Throws EffectCycleError if `candidate` cannot be ordered with the rest.
    void check_acyclic(const Effect& candidate) const;
    void invalidate_order();
    void rebuild_order() const;

    std::vector<EffectInstance> instances_;
    std::uint64_t next_handle_ = 1;

    // Evaluation order depends only on the set of effects, never on stat
    // values, so it is computed once per mutation and reused.
    mutable std::vector<std::size_t> order_;
    mutable bool order_valid_ = false;
};

} // namespace moba_sim
