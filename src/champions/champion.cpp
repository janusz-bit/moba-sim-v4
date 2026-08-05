#include "champions/champion.hpp"

#include <array>
#include <utility>

namespace moba_sim {

namespace {

// Maps a StatId to the (base, growth) field pair on ChampionData.
struct StatSpec {
    double ChampionData::* base;
    double ChampionData::* growth;
};

// Indexed by stat_index(). A table rather than a switch: the static_assert
// below turns a forgotten stat into a compile error, whereas a switch over
// StatId would only warn and then hit undefined behaviour at runtime.
constexpr std::array<StatSpec, kStatCount> kStatSpecs = {{
    {&ChampionData::health, &ChampionData::health_growth},
    {&ChampionData::health_regen, &ChampionData::health_regen_growth},
    {&ChampionData::resource, &ChampionData::resource_growth},
    {&ChampionData::resource_regen, &ChampionData::resource_regen_growth},
    {&ChampionData::attack_damage, &ChampionData::attack_damage_growth},
    {&ChampionData::attack_speed, &ChampionData::attack_speed_growth},
    {&ChampionData::armor, &ChampionData::armor_growth},
    {&ChampionData::magic_resist, &ChampionData::magic_resist_growth},
    {&ChampionData::movement_speed, &ChampionData::movement_speed_growth},
    {&ChampionData::attack_range, &ChampionData::attack_range_growth},
}};

static_assert(kStatSpecs.size() == kStatCount,
              "kStatSpecs is missing a StatId: every stat needs a base/growth field pair");

/// Source label for the champion's own base stat, e.g. "Ahri base, lvl 5".
std::string base_source(const ChampionData& data, int level) {
    const std::string name = data.name.empty() ? "(unnamed)" : data.name;
    return name + " base, lvl " + std::to_string(level);
}

} // namespace

double ChampionData::base_value(StatId stat, int level) const {
    const auto [base_ptr, growth_ptr] = kStatSpecs[stat_index(stat)];
    const int growth_steps = level > 1 ? level - 1 : 0;
    return (this->*base_ptr) + (this->*growth_ptr) * growth_steps;
}

Champion::Champion(const ChampionData& data, int level)
    : name(data.name), resource_type(data.resource_type), range_type(data.range_type), data_(data),
      level_(level > 1 ? level : 1) {}

void Champion::rebuild() const {
    stats_.clear();

    // 1. Champion base stats at the current level.
    const std::string source = base_source(data_, level_);
    for (StatId stat : kAllStats) {
        stats_[stat].add_base(data_.base_value(stat, level_), source);
    }

    // 2. Items, in equip order, labeled with the item's name unless the
    //    modifier carries its own label.
    for (const Item& item : items_) {
        for (const Modifier& mod : item.modifiers) {
            stats_.apply(mod, mod.source.empty() ? item.name : mod.source);
        }
    }

    // 3. Effects, in dependency order, on top of base + items. Pure: this may
    //    run any number of times and always produces the same table.
    effects_.contribute_all(stats_, now_, rate_);

    dirty_ = false;
}

const StatTable& Champion::stats() const {
    if (dirty_) {
        rebuild();
    }
    return stats_;
}

const StatPipeline& Champion::pipeline(StatId stat) const { return stats()[stat]; }

double Champion::compute(StatId stat) const { return stats().compute(stat); }

StatBreakdown Champion::explain(StatId stat) const { return stats().breakdown(stat); }

void Champion::set_level(int level) {
    level_ = level > 1 ? level : 1;
    invalidate();
}

void Champion::equip(const Item& item) {
    items_.push_back(item);
    invalidate();
}

bool Champion::unequip(const Item& item) { return unequip(item.name); }

bool Champion::unequip(const std::string& item_name) {
    for (auto it = items_.begin(); it != items_.end(); ++it) {
        if (it->name == item_name) {
            items_.erase(it);
            invalidate();
            return true;
        }
    }
    return false;
}

std::optional<EffectHandle> Champion::apply_effect(Effect effect, Tick now) {
    auto handle = effects_.apply(std::move(effect), now);
    invalidate();
    return handle;
}

std::optional<EffectHandle> Champion::apply_effect(Effect effect) {
    return apply_effect(std::move(effect), now_);
}

std::size_t Champion::remove_effect(const EffectKey& key) {
    const std::size_t removed = effects_.remove(key);
    if (removed > 0) {
        invalidate();
    }
    return removed;
}

bool Champion::remove_effect(EffectHandle handle) {
    const bool removed = effects_.remove(handle);
    if (removed) {
        invalidate();
    }
    return removed;
}

std::vector<EffectKey> Champion::advance_to(Tick now) {
    now_ = now;
    auto expired = effects_.advance(now_);
    // Time itself changes what effects contribute (a Timed effect on its last
    // tick, an effect scaling with elapsed time), so the table is always stale
    // after a step, even when nothing expired.
    invalidate();
    return expired;
}

std::vector<EffectKey> Champion::advance_by(TickSpan span) { return advance_to(now_ + span); }

void Champion::set_tick_rate(TickRate rate) {
    rate_ = rate;
    invalidate();
}

} // namespace moba_sim
