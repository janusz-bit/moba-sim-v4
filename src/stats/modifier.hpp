#pragma once

namespace moba_sim {

// Three classes of stat modifiers, modeled after Path of Exile's damage pipeline:
//   getStat(stat) = sum(Base) * (1 + sum(Inc)) * product(More)
//
// Base - added together:             10 + 20 + 30 = 60
// Inc  - additive, starting from 1.0: 1.0 + 0.1 + 0.2 = 1.3
// More - multiplicative:             1.1 * 1.2 * 1.3 = 1.716
enum class ModifierKind {
    Base,
    Inc,
    More,
};

struct Modifier {
    ModifierKind kind;
    double value;
};

} // namespace moba_sim