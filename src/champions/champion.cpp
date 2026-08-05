#include "champions/champion.hpp"

#include <utility>

namespace moba_sim {

namespace {

// Maps a StatId to the (base, growth) field pair on ChampionData.
struct StatSpec {
    double ChampionData::* base;
    double ChampionData::* growth;
};

constexpr StatSpec spec_for(StatId stat) {
    switch (stat) {
    case StatId::Health:
        return {&ChampionData::health, &ChampionData::health_growth};
    case StatId::HealthRegen:
        return {&ChampionData::health_regen, &ChampionData::health_regen_growth};
    case StatId::Resource:
        return {&ChampionData::resource, &ChampionData::resource_growth};
    case StatId::ResourceRegen:
        return {&ChampionData::resource_regen, &ChampionData::resource_regen_growth};
    case StatId::AttackDamage:
        return {&ChampionData::attack_damage, &ChampionData::attack_damage_growth};
    case StatId::AttackSpeed:
        return {&ChampionData::attack_speed, &ChampionData::attack_speed_growth};
    case StatId::Armor:
        return {&ChampionData::armor, &ChampionData::armor_growth};
    case StatId::MagicResist:
        return {&ChampionData::magic_resist, &ChampionData::magic_resist_growth};
    case StatId::MovementSpeed:
        return {&ChampionData::movement_speed, &ChampionData::movement_speed_growth};
    case StatId::AttackRange:
        return {&ChampionData::attack_range, &ChampionData::attack_range_growth};
    }
    std::unreachable();
}

void apply_modifier(StatPipeline& pipe, const ItemModifier& mod, const std::string& source) {
    switch (mod.kind) {
    case ModifierKind::Base:
        pipe.add_base(mod.value, source);
        break;
    case ModifierKind::Inc:
        pipe.add_inc(mod.value, source);
        break;
    case ModifierKind::More:
        pipe.add_more(mod.value, source);
        break;
    }
}

/// Source label for the champion's own base stat, e.g. "Ahri base, lvl 5".
std::string base_source(const ChampionData& data, int level) {
    const std::string name = data.name.empty() ? "(unnamed)" : data.name;
    return name + " base, lvl " + std::to_string(level);
}

} // namespace

double ChampionData::base_value(StatId stat, int level) const {
    const auto [base_ptr, growth_ptr] = spec_for(stat);
    const int growth_steps = level > 1 ? level - 1 : 0;
    return (this->*base_ptr) + (this->*growth_ptr) * growth_steps;
}

Champion::Champion(const ChampionData& data, int level)
    : name(data.name), resource_type(data.resource_type), range_type(data.range_type), data_(data),
      level_(level) {
    seed_pipelines();
}

void Champion::seed_pipelines() {
    const std::string source = base_source(data_, level_);
    for (StatId stat : kAllStats) {
        StatPipeline& pipe = pipelines_[stat_index(stat)];
        pipe = StatPipeline{};
        pipe.add_base(data_.base_value(stat, level_), source);
    }
}

StatPipeline& Champion::pipeline(StatId stat) { return pipelines_[stat_index(stat)]; }

const StatPipeline& Champion::pipeline(StatId stat) const { return pipelines_[stat_index(stat)]; }

double Champion::compute(StatId stat) const { return pipeline(stat).compute(); }

StatBreakdown Champion::explain(StatId stat) const { return pipeline(stat).breakdown(); }

void Champion::equip(const Item& item) {
    items_.push_back(item);
    for (const auto& mod : item.modifiers) {
        apply_modifier(pipeline(mod.stat), mod, item.name);
    }
}

void Champion::unequip(const Item& item) {
    for (auto it = items_.begin(); it != items_.end(); ++it) {
        if (it->name == item.name) {
            items_.erase(it);
            break;
        }
    }
    // Rebuild pipelines from base data + remaining items.
    seed_pipelines();
    for (const auto& equipped : items_) {
        for (const auto& mod : equipped.modifiers) {
            apply_modifier(pipeline(mod.stat), mod, equipped.name);
        }
    }
}

const std::vector<Item>& Champion::items() const { return items_; }

} // namespace moba_sim
