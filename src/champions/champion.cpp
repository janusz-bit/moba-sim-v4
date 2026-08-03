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
    health_.add({ModifierKind::Base, data.base_value(StatId::Health, level)});
    health_regen_.add({ModifierKind::Base, data.base_value(StatId::HealthRegen, level)});
    resource_.add({ModifierKind::Base, data.base_value(StatId::Resource, level)});
    resource_regen_.add({ModifierKind::Base, data.base_value(StatId::ResourceRegen, level)});
    attack_damage_.add({ModifierKind::Base, data.base_value(StatId::AttackDamage, level)});
    attack_speed_.add({ModifierKind::Base, data.base_value(StatId::AttackSpeed, level)});
    armor_.add({ModifierKind::Base, data.base_value(StatId::Armor, level)});
    magic_resist_.add({ModifierKind::Base, data.base_value(StatId::MagicResist, level)});
    movement_speed_.add({ModifierKind::Base, data.base_value(StatId::MovementSpeed, level)});
    attack_range_.add({ModifierKind::Base, data.base_value(StatId::AttackRange, level)});
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

void Champion::equip(const Item& item) {
    items_.push_back(item);
    for (const auto& mod : item.modifiers) {
        pipeline(mod.stat).add({mod.kind, mod.value});
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
    health_.add({ModifierKind::Base, data_.base_value(StatId::Health, level_)});
    health_regen_.add({ModifierKind::Base, data_.base_value(StatId::HealthRegen, level_)});
    resource_.add({ModifierKind::Base, data_.base_value(StatId::Resource, level_)});
    resource_regen_.add({ModifierKind::Base, data_.base_value(StatId::ResourceRegen, level_)});
    attack_damage_.add({ModifierKind::Base, data_.base_value(StatId::AttackDamage, level_)});
    attack_speed_.add({ModifierKind::Base, data_.base_value(StatId::AttackSpeed, level_)});
    armor_.add({ModifierKind::Base, data_.base_value(StatId::Armor, level_)});
    magic_resist_.add({ModifierKind::Base, data_.base_value(StatId::MagicResist, level_)});
    movement_speed_.add({ModifierKind::Base, data_.base_value(StatId::MovementSpeed, level_)});
    attack_range_.add({ModifierKind::Base, data_.base_value(StatId::AttackRange, level_)});
    for (const auto& equipped : items_) {
        for (const auto& mod : equipped.modifiers) {
            pipeline(mod.stat).add({mod.kind, mod.value});
        }
    }
}

const std::vector<Item>& Champion::items() const { return items_; }

} // namespace moba_sim