#pragma once

#include <array>
#include <cstddef>

namespace moba_sim {

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

/// Every StatId value, for iteration. Keep in sync with the enum above.
inline constexpr std::array kAllStats = {
    StatId::Health,        StatId::HealthRegen, StatId::Resource, StatId::ResourceRegen,
    StatId::AttackDamage,  StatId::AttackSpeed, StatId::Armor,    StatId::MagicResist,
    StatId::MovementSpeed, StatId::AttackRange,
};

/// Number of stats; sizes tables indexed by stat_index().
inline constexpr std::size_t kStatCount = kAllStats.size();

/// Index of `stat` in tables of size kStatCount.
[[nodiscard]] constexpr std::size_t stat_index(StatId stat) {
    return static_cast<std::size_t>(stat);
}

} // namespace moba_sim