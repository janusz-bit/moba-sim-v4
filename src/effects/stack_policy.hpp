#pragma once

#include <cstdint>

namespace moba_sim {

/// What happens when an effect is applied while one with the same EffectKey is
/// already active.
///
/// "Same key replaces the old one" covers only one of these. Real content needs
/// all of them, and picking the wrong one is a balance bug, so the choice is
/// explicit at the application site.
enum class StackPolicy {
    /// Reset the duration, keep one instance. The common buff re-application:
    /// re-casting a 3s buff at t=2 leaves one buff ending at t=5.
    Refresh,

    /// Add an intensity stack (up to Effect::max_stacks) and refresh the
    /// duration. The effect's contribution is multiplied by the stack count.
    Stack,

    /// Keep one instance and add the new duration to what is left, so nothing
    /// is wasted by early re-application.
    ExtendDuration,

    /// Do nothing while an instance is active. For effects that must not be
    /// re-triggered before they expire.
    IgnoreIfPresent,

    /// Replace only if the new instance contributes more (higher magnitude, or
    /// equal magnitude with a later expiry). Two different sources of the same
    /// aura where only the strongest applies.
    ReplaceIfStronger,

    /// Coexist as an independent instance. Distinct instances with the same key
    /// each keep their own lifetime and expire separately.
    Independent,
};

/// Number of intensity stacks on an effect instance.
using StackCount = std::uint32_t;

} // namespace moba_sim
