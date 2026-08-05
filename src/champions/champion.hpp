#pragma once

#include <array>
#include <string>
#include <vector>

#include "items/item.hpp"
#include "stats/stat_breakdown.hpp"
#include "stats/stat_id.hpp"
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

/// Champion base statistics in the LoL wiki format:
/// value at level 1 + growth added on each level-up.
/// See e.g. https://wiki.leagueoflegends.com/en-us/Ahri ("Base statistics").
struct ChampionData {
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

    /// Returns the base value for `stat` at the given `level`
    /// (base + growth * (level - 1)). At level 1 this is just the base value.
    [[nodiscard]] double base_value(StatId stat, int level = 1) const;
};

/// A live champion instance: each stat is a StatPipeline seeded from
/// ChampionData at a given level, ready to receive Inc/More modifiers
/// from items and buffs.
struct Champion {
    std::string name;
    ResourceType resource_type = ResourceType::Mana;
    RangeType range_type = RangeType::Melee;

    Champion() = default;

    /// Builds a champion from wiki data at the given `level`. Each stat's
    /// pipeline is seeded with `base + growth*(level-1)` as a single Base
    /// modifier.
    Champion(const ChampionData& data, int level = 1);

    /// Returns the pipeline for `stat`, so callers can add modifiers
    /// (items, buffs) and inspect the result.
    [[nodiscard]] StatPipeline& pipeline(StatId stat);
    [[nodiscard]] const StatPipeline& pipeline(StatId stat) const;

    /// Returns the fully computed value of `stat` (Base * Inc * More).
    [[nodiscard]] double compute(StatId stat) const;

    /// Returns the provenance of `stat`: every contributing modifier labeled
    /// with its source (champion base, item, ...), for debugging.
    [[nodiscard]] StatBreakdown explain(StatId stat) const;

    /// Equips `item` on the champion: stores it and pushes every modifier
    /// into the matching stat pipeline.
    void equip(const Item& item);

    /// Removes the first item equal to `item` (by name) and rebuilds the
    /// stat pipelines from the champion data and remaining items.
    void unequip(const Item& item);

    /// Returns the items currently equipped.
    [[nodiscard]] const std::vector<Item>& items() const;

  private:
    /// Resets every pipeline and seeds it with the champion's base stat
    /// (base + growth*(level-1)) as a single labeled Base modifier.
    void seed_pipelines();

    /// One pipeline per StatId, indexed by stat_index().
    std::array<StatPipeline, kStatCount> pipelines_;

    std::vector<Item> items_;

    ChampionData data_;
    int level_ = 1;
};

} // namespace moba_sim