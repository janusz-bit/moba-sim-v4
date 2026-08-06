#pragma once

#include <compare>
#include <string>

namespace moba_sim {

/// The stable identity of an effect: what applied it, and which effect it is.
///
/// Identity is *semantic*, not positional. Two applications of Ahri's Q
/// produce the same key, which is what lets EffectSet recognise a re-cast as
/// a refresh rather than a second, independent buff. Contrast an
/// auto-incrementing counter: the same buff cast twice would get two ids and
/// silently stack, and the numbers would depend on how many effects happened
/// to be created earlier in the run.
///
/// Being plain data also means keys are comparable, printable, hashable by
/// hand, and reproducible across runs — a prerequisite for deterministic
/// replay.
struct EffectKey {
    std::string source{}; ///< Who applied it: "Ahri", "Sheen", "Baron".
    std::string name{};   ///< Which effect: "Essence Theft", "Spellblade".

    /// Orders keys, so effects can live in sorted containers.
    [[nodiscard]] friend auto operator<=>(const EffectKey&, const EffectKey&) = default;
    /// Equality is what drives stacking: the same key means the same effect.
    [[nodiscard]] friend bool operator==(const EffectKey&, const EffectKey&) = default;

    /// Provenance label used in stat breakdowns, e.g. "Sheen (Spellblade)".
    /// Falls back to whichever half is present.
    [[nodiscard]] std::string label() const {
        if (source.empty()) {
            return name;
        }
        if (name.empty()) {
            return source;
        }
        return source + " (" + name + ")";
    }
};

} // namespace moba_sim
