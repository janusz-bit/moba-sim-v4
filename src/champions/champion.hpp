#pragma once

#include <string>

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
};

} // namespace moba_sim
