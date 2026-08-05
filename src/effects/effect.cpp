#include "effects/effect.hpp"

#include <cmath>
#include <utility>

#include "stats/stat_pipeline.hpp"

namespace moba_sim {

void ModifierSink::check(StatId stat) const {
    if (!writes_->contains(stat)) {
        throw UndeclaredStatAccess{*key_, stat, "writes"};
    }
}

std::string ModifierSink::label() const {
    const std::string base = key_->label();
    if (stacks_ <= 1) {
        return base;
    }
    return base + " x" + std::to_string(stacks_);
}

void ModifierSink::add_base(StatId stat, double value) {
    check(stat);
    (*table_)[stat].add_base(value * stacks_, label());
}

void ModifierSink::add_inc(StatId stat, double value) {
    check(stat);
    (*table_)[stat].add_inc(value * stacks_, label());
}

void ModifierSink::add_more(StatId stat, double value) {
    check(stat);
    // N stacks of a More modifier compound, matching "applied N times":
    // (1 + v)^N - 1 is the single increment with the same effect.
    const double compounded = std::pow(1.0 + value, static_cast<double>(stacks_)) - 1.0;
    (*table_)[stat].add_more(compounded, label());
}

void ModifierSink::add(const Modifier& mod) {
    switch (mod.kind) {
    case ModifierKind::Base:
        add_base(mod.stat, mod.value);
        break;
    case ModifierKind::Inc:
        add_inc(mod.stat, mod.value);
        break;
    case ModifierKind::More:
        add_more(mod.stat, mod.value);
        break;
    }
}

void ModifierSink::add_unstacked(StatId stat, ModifierKind kind, double value) {
    check(stat);
    apply_modifier((*table_)[stat], Modifier{stat, kind, value, {}}, label());
}

Effect flat_effect(EffectKey key, std::vector<Modifier> mods, Lifetime lifetime) {
    StatMask writes;
    for (const Modifier& mod : mods) {
        writes.set(mod.stat);
    }

    return Effect{
        .key = std::move(key),
        .reads = {},
        .writes = writes,
        .lifetime = std::move(lifetime),
        .policy = StackPolicy::Refresh,
        .magnitude = mods.empty() ? 0.0 : mods.front().value,
        .max_stacks = 1,
        .contribute =
            [mods = std::move(mods)](const EffectContext& /*ctx*/, ModifierSink& sink) {
                for (const Modifier& mod : mods) {
                    sink.add(mod);
                }
            },
    };
}

} // namespace moba_sim
