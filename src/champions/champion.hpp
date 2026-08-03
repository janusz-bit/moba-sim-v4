#pragma once

#include <string>

#include "stats/stat_pipeline.hpp"

namespace moba_sim {

// Resource bar used by the champion's abilities
enum class ResourceType {
    None,
    Mana,
    Energy,
    Fury,
};

enum class RangeType {
    Melee,
    Ranged,
};

// Identifies a champion stat that flows through the Base/Inc/More pipeline.
enum class StatId {
    Health,
    HealthRegen,
    Resource,
    ResourceRegen,
    AttackDamage,
    AttackSpeed,
    Armor,
    MagicResist,
    MovementSpeed,
    AttackRange,
};

struct Champion;

/// Maps a StatId to the (base, growth) field pair it refers to.
struct StatSpec {
    double Champion::* base;
    double Champion::* growth;
};

/// A League of Legends champion with base statistics in the wiki format:
/// value at level 1 + growth added on each level-up.
/// See e.g. https://wiki.leagueoflegends.com/en-us/Ahri ("Base statistics").
struct Champion {
    std::string name;

    ResourceType resource_type = ResourceType::Mana;
    RangeType range_type = RangeType::Melee;

    // Base stats (level 1)
    double health = 0.0;         // HP
    double health_regen = 0.0;   // HP5, per 5 seconds
    double resource = 0.0;       // MP (mana / energy / ...)
    double resource_regen = 0.0; // MP5, per 5 seconds
    double attack_damage = 0.0;  // AD
    double attack_speed = 0.0;   // attacks per second
    double armor = 0.0;          // AR
    double magic_resist = 0.0;   // MR
    double movement_speed = 0.0; // MS
    double attack_range = 0.0;   // in game units

    // Per-level growth
    double health_growth = 0.0;
    double health_regen_growth = 0.0;
    double resource_growth = 0.0;
    double resource_regen_growth = 0.0;
    double attack_damage_growth = 0.0;
    double attack_speed_growth = 0.0; // in % of base attack speed
    double armor_growth = 0.0;
    double magic_resist_growth = 0.0;
    double movement_speed_growth = 0.0;
    double attack_range_growth = 0.0;

    /// Returns the champion's base value for `stat` at the given `level`
    /// (base + growth * (level - 1)). At level 1 this is just the base value.
    [[nodiscard]] double base_value(StatId stat, int level = 1) const;

    /// Returns a pipeline seeded with the champion's base value for `stat`
    /// at the given `level` as a single Base modifier. Callers can add
    /// Inc/More modifiers (items, buffs) and then call compute().
    [[nodiscard]] StatPipeline pipeline_for(StatId stat, int level = 1) const;
};

/// Returns the (base, growth) field pointers for the given stat.
/// Every stat uses the same `base + growth * (level - 1)` formula.
[[nodiscard]] constexpr StatSpec spec_for(StatId stat) {
    switch (stat) {
    case StatId::Health:
        return {&Champion::health, &Champion::health_growth};
    case StatId::HealthRegen:
        return {&Champion::health_regen, &Champion::health_regen_growth};
    case StatId::Resource:
        return {&Champion::resource, &Champion::resource_growth};
    case StatId::ResourceRegen:
        return {&Champion::resource_regen, &Champion::resource_regen_growth};
    case StatId::AttackDamage:
        return {&Champion::attack_damage, &Champion::attack_damage_growth};
    case StatId::AttackSpeed:
        return {&Champion::attack_speed, &Champion::attack_speed_growth};
    case StatId::Armor:
        return {&Champion::armor, &Champion::armor_growth};
    case StatId::MagicResist:
        return {&Champion::magic_resist, &Champion::magic_resist_growth};
    case StatId::MovementSpeed:
        return {&Champion::movement_speed, &Champion::movement_speed_growth};
    case StatId::AttackRange:
        return {&Champion::attack_range, &Champion::attack_range_growth};
    }
    return {nullptr, nullptr};
}

} // namespace moba_sim