#include "champions/champion.hpp"

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
    return {nullptr, nullptr};
}

} // namespace

namespace {

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

void seed_pipeline(StatPipeline& pipe, const ChampionData& data, StatId stat, int level) {
    pipe.add_base(data.base_value(stat, level), base_source(data, level));
}

} // namespace

double ChampionData::base_value(StatId stat, int level) const {
    const auto [base_ptr, growth_ptr] = spec_for(stat);
    if (base_ptr == nullptr) {
        return 0.0;
    }
    const int growth_steps = level > 1 ? level - 1 : 0;
    return (this->*base_ptr) + (this->*growth_ptr) * growth_steps;
}

Champion::Champion(const ChampionData& data, int level)
    : name(data.name), resource_type(data.resource_type), range_type(data.range_type), data_(data),
      level_(level) {
    seed_pipeline(health_, data, StatId::Health, level);
    seed_pipeline(health_regen_, data, StatId::HealthRegen, level);
    seed_pipeline(resource_, data, StatId::Resource, level);
    seed_pipeline(resource_regen_, data, StatId::ResourceRegen, level);
    seed_pipeline(attack_damage_, data, StatId::AttackDamage, level);
    seed_pipeline(attack_speed_, data, StatId::AttackSpeed, level);
    seed_pipeline(armor_, data, StatId::Armor, level);
    seed_pipeline(magic_resist_, data, StatId::MagicResist, level);
    seed_pipeline(movement_speed_, data, StatId::MovementSpeed, level);
    seed_pipeline(attack_range_, data, StatId::AttackRange, level);
}

StatPipeline& Champion::pipeline(StatId stat) {
    switch (stat) {
    case StatId::Health:
        return health_;
    case StatId::HealthRegen:
        return health_regen_;
    case StatId::Resource:
        return resource_;
    case StatId::ResourceRegen:
        return resource_regen_;
    case StatId::AttackDamage:
        return attack_damage_;
    case StatId::AttackSpeed:
        return attack_speed_;
    case StatId::Armor:
        return armor_;
    case StatId::MagicResist:
        return magic_resist_;
    case StatId::MovementSpeed:
        return movement_speed_;
    case StatId::AttackRange:
        return attack_range_;
    }
    return health_; // unreachable
}

const StatPipeline& Champion::pipeline(StatId stat) const {
    switch (stat) {
    case StatId::Health:
        return health_;
    case StatId::HealthRegen:
        return health_regen_;
    case StatId::Resource:
        return resource_;
    case StatId::ResourceRegen:
        return resource_regen_;
    case StatId::AttackDamage:
        return attack_damage_;
    case StatId::AttackSpeed:
        return attack_speed_;
    case StatId::Armor:
        return armor_;
    case StatId::MagicResist:
        return magic_resist_;
    case StatId::MovementSpeed:
        return movement_speed_;
    case StatId::AttackRange:
        return attack_range_;
    }
    return health_; // unreachable
}

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
    for (auto* pipe : {&health_, &health_regen_, &resource_, &resource_regen_, &attack_damage_,
                       &attack_speed_, &armor_, &magic_resist_, &movement_speed_, &attack_range_}) {
        *pipe = StatPipeline{};
    }
    seed_pipeline(health_, data_, StatId::Health, level_);
    seed_pipeline(health_regen_, data_, StatId::HealthRegen, level_);
    seed_pipeline(resource_, data_, StatId::Resource, level_);
    seed_pipeline(resource_regen_, data_, StatId::ResourceRegen, level_);
    seed_pipeline(attack_damage_, data_, StatId::AttackDamage, level_);
    seed_pipeline(attack_speed_, data_, StatId::AttackSpeed, level_);
    seed_pipeline(armor_, data_, StatId::Armor, level_);
    seed_pipeline(magic_resist_, data_, StatId::MagicResist, level_);
    seed_pipeline(movement_speed_, data_, StatId::MovementSpeed, level_);
    seed_pipeline(attack_range_, data_, StatId::AttackRange, level_);
    for (const auto& equipped : items_) {
        for (const auto& mod : equipped.modifiers) {
            apply_modifier(pipeline(mod.stat), mod, equipped.name);
        }
    }
}

const std::vector<Item>& Champion::items() const { return items_; }

} // namespace moba_sim