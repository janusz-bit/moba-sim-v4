#pragma once

#include <string>
#include <vector>

#include "stats/modifier.hpp"
#include "stats/stat_id.hpp"

namespace moba_sim {

/// A single stat modification: which stat, what kind, how much.
struct ItemModifier {
    StatId stat;
    ModifierKind kind;
    double value;
};

/// An equippable item: a named collection of stat modifiers.
struct Item {
    std::string name;
    std::vector<ItemModifier> modifiers;

    /// Returns the modifiers this item grants for the given stat.
    [[nodiscard]] std::vector<ItemModifier> modifiers_for(StatId stat) const;
};

} // namespace moba_sim