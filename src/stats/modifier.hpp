#pragma once

#include <string>
#include <utility>

#include "stats/stat_id.hpp"

namespace moba_sim {

class StatPipeline;

// Three classes of stat modifiers, modeled after Path of Exile's damage pipeline:
//   getStat(stat) = sum(Base) * (1 + sum(Inc)) * product(More)
//
// Base - added together:             10 + 20 + 30 = 60
// Inc  - additive, starting from 1.0: 1.0 + 0.1 + 0.2 = 1.3
// More - multiplicative:             1.1 * 1.2 * 1.3 = 1.716
//
// Each bucket is a separate vector inside StatPipeline; the ModifierKind tag
// tells StatPipeline which bucket to target.

// Tags identifying which bucket a modifier belongs to.
enum class ModifierKind {
    Base,
    Inc,
    More,
};

/// A single stat modification, independent of where it came from: items,
/// effects and champion base stats all produce these. `source` is the
/// provenance label that shows up in StatBreakdown / format_breakdown.
///
/// Keeping one modifier type for every source is what lets Champion replay
/// its whole modifier set from the sources it owns (see Champion::rebuild).
struct Modifier {
    StatId stat{};
    ModifierKind kind = ModifierKind::Base;
    double value = 0.0;
    std::string source{};
};

/// Convenience constructors. They exist so call sites read as
/// `base_mod(StatId::Health, 150, "Ruby Crystal")` instead of an aggregate
/// initializer whose field order has to be remembered.
[[nodiscard]] inline Modifier base_mod(StatId stat, double value, std::string source = "") {
    return {stat, ModifierKind::Base, value, std::move(source)};
}

[[nodiscard]] inline Modifier inc_mod(StatId stat, double value, std::string source = "") {
    return {stat, ModifierKind::Inc, value, std::move(source)};
}

[[nodiscard]] inline Modifier more_mod(StatId stat, double value, std::string source = "") {
    return {stat, ModifierKind::More, value, std::move(source)};
}

/// Pushes `mod` into the bucket of `pipe` named by `mod.kind`. This is the one
/// place that maps ModifierKind onto the StatPipeline::add_* overloads; every
/// modifier source goes through it so the mapping cannot drift per call site.
void apply_modifier(StatPipeline& pipe, const Modifier& mod);

/// Same, but overrides the provenance label — used when the source name is
/// known by the container rather than the modifier (e.g. an item's name).
void apply_modifier(StatPipeline& pipe, const Modifier& mod, const std::string& source);

} // namespace moba_sim
