#include "effects/lifetime.hpp"

namespace moba_sim {

bool is_alive(const Lifetime& lifetime, Tick now) {
    return std::visit(
        [now](const auto& value) -> bool {
            using T = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<T, Timed>) {
                return now < value.expires_at;
            } else if constexpr (std::is_same_v<T, Until>) {
                // A predicate-less Until would silently live forever; treat it
                // as expired so a half-built effect fails loudly instead.
                return value.alive && value.alive(now);
            } else {
                // Permanent lives until removed; OneShot is retired by advance()
                // after it has contributed, not by this predicate.
                return true;
            }
        },
        lifetime);
}

std::optional<TickSpan> remaining(const Lifetime& lifetime, Tick now) {
    if (const auto* timed = std::get_if<Timed>(&lifetime)) {
        const TickSpan left = timed->expires_at - now;
        return left.count() > 0 ? left : TickSpan{0};
    }
    return std::nullopt;
}

} // namespace moba_sim
