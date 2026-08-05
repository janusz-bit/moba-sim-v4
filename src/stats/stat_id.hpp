#pragma once

#include <array>
#include <cstddef>
#include <string_view>
#include <utility>

namespace moba_sim {

/// Identifies a champion stat that flows through the Base/Inc/More pipeline.
///
/// `Count` is a sentinel, not a stat: it exists so kStatCount, kAllStats and
/// every kStatCount-sized table below grow automatically when a stat is added.
/// Never switch on StatId and never store it as a value — see kStatNames and
/// champion.cpp's stat table for the pattern (a table with a static_assert,
/// which turns a forgotten entry into a compile error instead of runtime UB).
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

    Count, // sentinel — keep last
};

/// Number of real stats; sizes every table indexed by stat_index().
inline constexpr std::size_t kStatCount = static_cast<std::size_t>(StatId::Count);

/// Index of `stat` in tables of size kStatCount.
[[nodiscard]] constexpr std::size_t stat_index(StatId stat) {
    return static_cast<std::size_t>(stat);
}

namespace detail {

template <std::size_t... Is>
constexpr std::array<StatId, sizeof...(Is)> make_all_stats(std::index_sequence<Is...> /*seq*/) {
    return {static_cast<StatId>(Is)...};
}

} // namespace detail

/// Every StatId value except the sentinel, for iteration. Derived from the
/// enum, so it can never fall out of sync with it.
inline constexpr std::array<StatId, kStatCount> kAllStats =
    detail::make_all_stats(std::make_index_sequence<kStatCount>{});

/// Display names, indexed by stat_index(). Adding a StatId without adding a
/// name here fails the static_assert below.
inline constexpr std::array<std::string_view, kStatCount> kStatNames = {
    "Health",      "HealthRegen", "Resource",    "ResourceRegen", "AttackDamage",
    "AttackSpeed", "Armor",       "MagicResist", "MovementSpeed", "AttackRange",
};

static_assert(kStatNames.size() == kStatCount, "kStatNames is missing a StatId entry");

/// Returns the display name of `stat`, e.g. "AttackDamage".
[[nodiscard]] constexpr std::string_view stat_name(StatId stat) {
    return kStatNames[stat_index(stat)];
}

} // namespace moba_sim
