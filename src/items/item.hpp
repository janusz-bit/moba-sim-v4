#pragma once

#include <string>
#include <vector>

#include "champions/champion.hpp"
#include "stats/modifier.hpp"

namespace moba_sim {

/// A single stat modification: which stat, what kind, how much.
struct ItemModifier {
    StatId stat;
    ModifierKind kind;
    double value;
};

/// An equippable item: a named collection of stat modifiers.
/// Apply it to a Champion to push every modifier into the matching
/// stat pipeline.
struct Item {
    std::string name;
    std::vector<ItemModifier> modifiers;

    /// Returns the modifiers this item grants for the given stat.
    [[nodiscard]] std::vector<ItemModifier> modifiers_for(StatId stat) const;

    /// Pushes every modifier of this item into the champion's matching
    /// stat pipeline.
    void apply_to(Champion& champion) const;
};

} // namespace moba_sim