#pragma once

namespace moba_sim {

// Three classes of stat modifiers, modeled after Path of Exile's damage pipeline:
//   getStat(stat) = sum(Base) * (1 + sum(Inc)) * product(More)
//
// Base - added together:             10 + 20 + 30 = 60
// Inc  - additive, starting from 1.0: 1.0 + 0.1 + 0.2 = 1.3
// More - multiplicative:             1.1 * 1.2 * 1.3 = 1.716
//
// Each bucket is a separate vector inside StatPipeline. Items describe their
// modifiers with ItemModifier (items/item.hpp), which carries a ModifierKind
// tag so StatPipeline knows which bucket to target.

// Tags identifying which bucket a modifier belongs to.
enum class ModifierKind {
    Base,
    Inc,
    More,
};

} // namespace moba_sim