#include "effects/effect_set.hpp"

#include <algorithm>
#include <utility>

namespace moba_sim {

namespace {

/// A node in the dependency graph: an effect reduced to what it depends on and
/// what it changes.
struct Node {
    const StatMask* reads;
    const StatMask* writes;
    const EffectKey* key;
};

/// True if `after` must be evaluated after `before` — i.e. `after` reads a stat
/// whose value `before` is responsible for.
///
/// The rule is per stat, with one deliberate exception. For a stat S that
/// `after` reads and `before` writes there is an edge, *unless* both effects
/// read and write S. In that case they are peers: two amplifiers of the same
/// stat ("Rabadon-style"), neither of which can sensibly wait for the other.
/// Ordering peers would report a false cycle for content that is perfectly
/// well-defined.
///
/// The exception is narrow on purpose. A genuine conversion cycle — one effect
/// turning AD into AP while another turns AP into AD — has each effect reading
/// a stat it does *not* write, so both edges survive and the cycle is still
/// caught.
bool depends_on(const Node& after, const Node& before) {
    for (StatId stat : kAllStats) {
        if (!after.reads->contains(stat) || !before.writes->contains(stat)) {
            continue;
        }
        const bool peers = after.writes->contains(stat) && before.reads->contains(stat);
        if (!peers) {
            return true;
        }
    }
    return false;
}

/// Deterministic topological sort (Kahn, always taking the lowest ready index).
/// Returns false and leaves `out` holding the nodes it managed to order when
/// the graph has a cycle.
///
/// Determinism matters: evaluation order must not depend on hash iteration or
/// pointer values, or two runs of the same scenario could produce different
/// numbers when More modifiers are involved.
bool topological_order(const std::vector<Node>& nodes, std::vector<std::size_t>& out) {
    const std::size_t count = nodes.size();
    std::vector<std::size_t> in_degree(count, 0);

    for (std::size_t after = 0; after < count; ++after) {
        for (std::size_t before = 0; before < count; ++before) {
            if (after != before && depends_on(nodes[after], nodes[before])) {
                ++in_degree[after];
            }
        }
    }

    out.clear();
    out.reserve(count);
    std::vector<bool> emitted(count, false);

    for (std::size_t step = 0; step < count; ++step) {
        // Lowest ready index — the tie-break that makes the order reproducible.
        std::size_t ready = count;
        for (std::size_t i = 0; i < count; ++i) {
            if (!emitted[i] && in_degree[i] == 0) {
                ready = i;
                break;
            }
        }
        if (ready == count) {
            return false; // everything left is part of a cycle
        }

        emitted[ready] = true;
        out.push_back(ready);

        for (std::size_t after = 0; after < count; ++after) {
            if (!emitted[after] && after != ready && depends_on(nodes[after], nodes[ready])) {
                --in_degree[after];
            }
        }
    }

    return true;
}

/// "'Ahri (Q)' -> 'Sheen (Spellblade)'" for the cycle error message.
std::string describe_unordered(const std::vector<Node>& nodes,
                               const std::vector<std::size_t>& ordered) {
    std::vector<bool> emitted(nodes.size(), false);
    for (std::size_t index : ordered) {
        emitted[index] = true;
    }

    std::string detail;
    for (std::size_t i = 0; i < nodes.size(); ++i) {
        if (!emitted[i]) {
            if (!detail.empty()) {
                detail += " -> ";
            }
            detail += "'" + nodes[i].key->label() + "'";
        }
    }
    return detail;
}

/// True if `policy` resolves a key collision by acting on the existing
/// instance instead of adding a second one.
bool replaces_existing(StackPolicy policy) { return policy != StackPolicy::Independent; }

} // namespace

std::vector<EffectInstance>::iterator EffectSet::find_mut(const EffectKey& key) {
    return std::find_if(instances_.begin(), instances_.end(),
                        [&key](const EffectInstance& inst) { return inst.effect.key == key; });
}

const EffectInstance* EffectSet::find(const EffectKey& key) const {
    const auto it =
        std::find_if(instances_.begin(), instances_.end(),
                     [&key](const EffectInstance& inst) { return inst.effect.key == key; });
    return it == instances_.end() ? nullptr : &*it;
}

void EffectSet::check_acyclic(const Effect& candidate) const {
    std::vector<Node> nodes;
    nodes.reserve(instances_.size() + 1);

    const bool candidate_replaces = replaces_existing(candidate.policy);
    for (const EffectInstance& inst : instances_) {
        // An instance the candidate is about to replace is not part of the
        // resulting graph, so it must not be checked against the candidate.
        if (candidate_replaces && inst.effect.key == candidate.key) {
            continue;
        }
        nodes.push_back({&inst.effect.reads, &inst.effect.writes, &inst.effect.key});
    }
    nodes.push_back({&candidate.reads, &candidate.writes, &candidate.key});

    std::vector<std::size_t> ordered;
    if (!topological_order(nodes, ordered)) {
        throw EffectCycleError{describe_unordered(nodes, ordered)};
    }
}

std::optional<EffectHandle> EffectSet::apply(Effect effect, Tick now) {
    const auto existing = find_mut(effect.key);
    const bool present = existing != instances_.end();

    if (present && effect.policy == StackPolicy::IgnoreIfPresent) {
        return std::nullopt;
    }

    if (present && effect.policy == StackPolicy::ReplaceIfStronger) {
        const bool weaker = effect.magnitude < existing->effect.magnitude;
        const bool equal_magnitude = effect.magnitude == existing->effect.magnitude;
        if (weaker) {
            return std::nullopt;
        }
        if (equal_magnitude) {
            // Same strength: only a later expiry is an improvement.
            const auto incoming = remaining(effect.lifetime, now);
            const auto current = remaining(existing->effect.lifetime, now);
            const bool longer =
                incoming.has_value() && current.has_value() && incoming->count() > current->count();
            const bool becomes_endless = !incoming.has_value() && current.has_value();
            if (!longer && !becomes_endless) {
                return std::nullopt;
            }
        }
    }

    check_acyclic(effect);
    invalidate_order();

    if (!present || effect.policy == StackPolicy::Independent) {
        const auto handle = static_cast<EffectHandle>(next_handle_++);
        instances_.push_back({handle, std::move(effect), 1, now});
        return handle;
    }

    // From here on the existing instance is updated in place, keeping its
    // handle valid across a refresh.
    switch (effect.policy) {
    case StackPolicy::Stack: {
        const StackCount cap = std::max<StackCount>(effect.max_stacks, 1);
        existing->stacks = std::min<StackCount>(existing->stacks + 1, cap);
        existing->effect = std::move(effect);
        existing->applied_at = now;
        break;
    }
    case StackPolicy::ExtendDuration: {
        const auto added = remaining(effect.lifetime, now);
        const auto left = remaining(existing->effect.lifetime, now);
        Lifetime extended = effect.lifetime;
        if (added.has_value() && left.has_value()) {
            // Nothing is wasted by re-applying early: the leftover is kept.
            extended = Timed{now + (*left + *added)};
        }
        existing->effect = std::move(effect);
        existing->effect.lifetime = std::move(extended);
        existing->applied_at = now;
        break;
    }
    case StackPolicy::Refresh:
    case StackPolicy::ReplaceIfStronger:
        existing->stacks = 1;
        existing->effect = std::move(effect);
        existing->applied_at = now;
        break;
    case StackPolicy::IgnoreIfPresent:
    case StackPolicy::Independent:
        break; // handled above
    }

    return existing->handle;
}

std::size_t EffectSet::remove(const EffectKey& key) {
    const auto new_end =
        std::remove_if(instances_.begin(), instances_.end(),
                       [&key](const EffectInstance& inst) { return inst.effect.key == key; });
    const auto removed = static_cast<std::size_t>(std::distance(new_end, instances_.end()));
    instances_.erase(new_end, instances_.end());
    if (removed > 0) {
        invalidate_order();
    }
    return removed;
}

bool EffectSet::remove(EffectHandle handle) {
    const auto it =
        std::find_if(instances_.begin(), instances_.end(),
                     [handle](const EffectInstance& inst) { return inst.handle == handle; });
    if (it == instances_.end()) {
        return false;
    }
    instances_.erase(it);
    invalidate_order();
    return true;
}

std::size_t EffectSet::remove_if(const std::function<bool(const EffectKey&)>& pred) {
    const auto new_end =
        std::remove_if(instances_.begin(), instances_.end(),
                       [&pred](const EffectInstance& inst) { return pred(inst.effect.key); });
    const auto removed = static_cast<std::size_t>(std::distance(new_end, instances_.end()));
    instances_.erase(new_end, instances_.end());
    if (removed > 0) {
        invalidate_order();
    }
    return removed;
}

void EffectSet::clear() {
    instances_.clear();
    invalidate_order();
}

std::vector<EffectKey> EffectSet::advance(Tick now) {
    std::vector<EffectKey> expired;

    const auto is_finished = [now](const EffectInstance& inst) {
        // A OneShot contributes during the step it was applied on and is
        // retired at the start of the next one. Deciding this from applied_at
        // keeps the evaluation phase free of side effects — nothing has to
        // "mark" the effect as spent, so stats can be recomputed any number of
        // times without changing what expires.
        if (std::holds_alternative<OneShot>(inst.effect.lifetime)) {
            return now > inst.applied_at;
        }
        return !is_alive(inst.effect.lifetime, now);
    };

    for (const EffectInstance& inst : instances_) {
        if (is_finished(inst)) {
            expired.push_back(inst.effect.key);
        }
    }

    if (!expired.empty()) {
        instances_.erase(std::remove_if(instances_.begin(), instances_.end(), is_finished),
                         instances_.end());
        invalidate_order();
    }

    return expired;
}

void EffectSet::contribute_all(StatTable& table, Tick now, TickRate rate) const {
    for (std::size_t index : evaluation_order()) {
        const EffectInstance& inst = instances_[index];
        if (!inst.effect.contribute) {
            continue;
        }

        const EffectContext ctx{
            .stats = StatView{table, inst.effect.reads, inst.effect.key},
            .now = now,
            .rate = rate,
            .stacks = inst.stacks,
        };
        ModifierSink sink{table, inst.effect.writes, inst.effect.key, inst.stacks};
        inst.effect.contribute(ctx, sink);
    }
}

const std::vector<std::size_t>& EffectSet::evaluation_order() const {
    if (!order_valid_) {
        rebuild_order();
    }
    return order_;
}

void EffectSet::invalidate_order() { order_valid_ = false; }

void EffectSet::rebuild_order() const {
    std::vector<Node> nodes;
    nodes.reserve(instances_.size());
    for (const EffectInstance& inst : instances_) {
        nodes.push_back({&inst.effect.reads, &inst.effect.writes, &inst.effect.key});
    }

    // apply() rejects cycles, so this cannot fail; if it somehow did, the
    // partial order is still a usable fallback.
    topological_order(nodes, order_);
    order_valid_ = true;
}

StackCount EffectSet::stacks_of(const EffectKey& key) const {
    const EffectInstance* inst = find(key);
    return inst == nullptr ? 0 : inst->stacks;
}

std::optional<TickSpan> EffectSet::remaining_on(const EffectKey& key, Tick now) const {
    const EffectInstance* inst = find(key);
    if (inst == nullptr) {
        return std::nullopt;
    }
    return remaining(inst->effect.lifetime, now);
}

StatMask EffectSet::written_stats() const {
    StatMask mask;
    for (const EffectInstance& inst : instances_) {
        mask |= inst.effect.writes;
    }
    return mask;
}

} // namespace moba_sim
