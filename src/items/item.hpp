#pragma once

#include <string>
#include <vector>

#include "stats/modifier.hpp"
#include "stats/stat_id.hpp"

namespace moba_sim {

/// An equippable item: a named collection of stat modifiers.
///
/// Modifiers are plain `Modifier` values — the same type effects produce — so
/// Champion applies both through one code path. An item modifier's `source`
/// label is normally left empty and filled in with the item's name when it is
/// equipped; set it explicitly to attribute a single line to something more
/// specific (e.g. "Zeal (passive)").
struct Item {
    std::string name{};                ///< Item name; labels its modifiers.
    std::vector<Modifier> modifiers{}; ///< The stat changes this item grants.

    /// Returns the modifiers this item grants for the given stat.
    [[nodiscard]] std::vector<Modifier> modifiers_for(StatId stat) const;
};

} // namespace moba_sim
