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

    /// Returns the champion's base value for `stat` at the given `level`
    /// (base + growth * (level - 1)). At level 1 this is just the base value.
    [[nodiscard]] double base_value(StatId stat, int level = 1) const;

    /// Returns a pipeline seeded with the champion's base value for `stat`
    /// at the given `level` as a single Base modifier. Callers can add
    /// Inc/More modifiers (items, buffs) and then call compute().
    [[nodiscard]] StatPipeline pipeline_for(StatId stat, int level = 1) const;
};

} // namespace moba_sim