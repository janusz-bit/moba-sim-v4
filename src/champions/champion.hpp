#pragma once

#include <optional>
#include <string>
#include <vector>

#include "effects/effect.hpp"
#include "effects/effect_set.hpp"
#include "items/item.hpp"
#include "sim/tick.hpp"
#include "stats/stat_breakdown.hpp"
#include "stats/stat_id.hpp"
#include "stats/stat_pipeline.hpp"
#include "stats/stat_table.hpp"

namespace moba_sim {

/// Resource bar used by the champion's abilities.
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
    std::string name{};

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

/// A live champion: base stats from ChampionData at a level, plus items, plus
/// effects, resolved into one StatTable.
///
/// Every modifier comes from a source the champion owns — base data, `items()`,
/// or `effects()` — so the stat table can always be rebuilt from scratch. That
/// is why there is no way to push a modifier straight into a pipeline: such a
/// modifier would have no owner and would silently vanish the next time
/// anything changed. Temporary bonuses are effects; permanent gear is items.
///
/// Stats are recomputed lazily: mutating the champion marks the table dirty and
/// the next read rebuilds it. Reads are const and cheap when nothing changed.
class Champion {
  public:
    std::string name;
    ResourceType resource_type = ResourceType::Mana;
    RangeType range_type = RangeType::Melee;

    Champion() = default;

    /// Builds a champion from wiki data at the given `level`.
    explicit Champion(const ChampionData& data, int level = 1);

    // --- stats -------------------------------------------------------------

    /// Fully computed value of `stat` (Base * Inc * More), including items and
    /// every live effect.
    [[nodiscard]] double compute(StatId stat) const;

    /// Provenance of `stat`: every contributing modifier labeled with its
    /// source (champion base, item name, effect key), for debugging.
    [[nodiscard]] StatBreakdown explain(StatId stat) const;

    /// Read-only view of the resolved pipeline for `stat`.
    [[nodiscard]] const StatPipeline& pipeline(StatId stat) const;

    /// The whole resolved stat table.
    [[nodiscard]] const StatTable& stats() const;

    // --- level -------------------------------------------------------------

    [[nodiscard]] int level() const { return level_; }

    /// Sets the level and re-seeds base stats; items and effects survive.
    void set_level(int level);

    /// The wiki data this champion was built from.
    [[nodiscard]] const ChampionData& data() const { return data_; }

    // --- items -------------------------------------------------------------

    /// Equips `item`; its modifiers are labeled with the item's name.
    void equip(const Item& item);

    /// Removes the first item with the same name. Returns true if one was found.
    bool unequip(const Item& item);
    bool unequip(const std::string& item_name);

    [[nodiscard]] const std::vector<Item>& items() const { return items_; }

    // --- effects -----------------------------------------------------------

    /// Applies `effect` at `now`, following its stack policy. Returns the live
    /// instance's handle, or nullopt if the policy dropped the application.
    std::optional<EffectHandle> apply_effect(Effect effect, Tick now);

    /// Convenience overload using the champion's current time.
    std::optional<EffectHandle> apply_effect(Effect effect);

    /// Removes every effect with `key`. Returns how many went away.
    std::size_t remove_effect(const EffectKey& key);

    /// Removes one specific instance.
    bool remove_effect(EffectHandle handle);

    [[nodiscard]] const EffectSet& effects() const { return effects_; }

    // --- time --------------------------------------------------------------

    /// Advances simulation time to `now`, expiring finished effects. Returns
    /// the keys that expired, so the caller can react to them.
    ///
    /// This is the champion's only mutating time step; reading stats never
    /// changes anything, so stats can be queried as often as needed.
    std::vector<EffectKey> advance_to(Tick now);

    /// Advances by `span` from the current time.
    std::vector<EffectKey> advance_by(TickSpan span);

    [[nodiscard]] Tick now() const { return now_; }

    /// Tick/second conversion used when effects ask for seconds.
    [[nodiscard]] TickRate tick_rate() const { return rate_; }
    void set_tick_rate(TickRate rate);

  private:
    /// Rebuilds the stat table from base data, then items, then effects.
    void rebuild() const;
    void invalidate() { dirty_ = true; }

    ChampionData data_;
    int level_ = 1;

    std::vector<Item> items_;
    EffectSet effects_;

    Tick now_ = kSimulationStart;
    TickRate rate_ = kDefaultTickRate;

    // Derived state: a pure function of the members above.
    mutable StatTable stats_;
    mutable bool dirty_ = true;
};

} // namespace moba_sim
