#pragma once

#include <functional>
#include <optional>
#include <variant>

#include "sim/tick.hpp"

namespace moba_sim {

/// How long an effect lives — expressed as data the framework can inspect,
/// not as logic hidden inside a callable.
///
/// The alternative (letting each effect decide its own lifetime by returning
/// `alive = false`) looks simpler but costs a lot: nothing outside the effect
/// can extend a duration, dispel a buff, show remaining time in a UI, or
/// schedule the next expiry, and the `now - start >= duration` comparison gets
/// copy-pasted into every timed effect. Worse, the start time then lives in a
/// lambda capture, which cannot be serialised or copied predictably.
///
/// With lifetime as data all of that is framework work done once. Effects that
/// genuinely need custom logic use Lifetime::Until, which is an escape hatch
/// rather than the only mechanism.

/// Lives until explicitly removed. Items' passives, auras while in range.
struct Permanent {};

/// Expires at a fixed point in time. Constructed from a start tick and a
/// duration so `expires_at` is computed once, at application time.
struct Timed {
    Tick expires_at{}; ///< The tick at which the effect stops being alive.

    /// Builds a lifetime ending `duration` after `now`, so the deadline is
    /// computed once, at application time.
    [[nodiscard]] static Timed for_span(Tick now, TickSpan duration) {
        return Timed{now + duration};
    }
};

/// Contributes on exactly one evaluation, then goes away. Useful for
/// one-frame effects such as a single empowered attack.
struct OneShot {};

/// Removed when the predicate stops holding. The escape hatch for lifetimes
/// that are neither permanent nor a plain duration ("while below 50% HP").
/// Prefer the cases above: a predicate is opaque to the scheduler, so it must
/// be polled on every step.
struct Until {
    /// Returns true while the effect should stay alive.
    std::function<bool(Tick now)> alive{};
};

/// One of Permanent, Timed, OneShot or Until — the whole vocabulary of how
/// long an effect can live.
using Lifetime = std::variant<Permanent, Timed, OneShot, Until>;

/// True if `lifetime` is still alive at `now`. OneShot reports alive here; it
/// is retired by the advance step after it has contributed once.
[[nodiscard]] bool is_alive(const Lifetime& lifetime, Tick now);

/// Remaining duration at `now`, or nullopt when the lifetime has no end time
/// the framework can know (Permanent, OneShot, Until). This is what a buff bar
/// reads — impossible to answer if the deadline hides in a closure.
[[nodiscard]] std::optional<TickSpan> remaining(const Lifetime& lifetime, Tick now);

} // namespace moba_sim
