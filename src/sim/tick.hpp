#pragma once

#include <cmath>
#include <compare>
#include <cstdint>

namespace moba_sim {

/// Simulation time is counted in whole ticks, never in seconds-as-double.
///
/// Why integers: a buff that expires at `start + 3s` has to expire at exactly
/// one well-defined tick. With accumulated floating-point dt the comparison
/// `now >= expires_at` drifts, so the same scenario can produce a different
/// number of buffed ticks depending on frame pacing. Integer ticks make expiry
/// exact, replay bit-reproducible, and tests free of epsilon fudging.
///
/// Seconds only appear at the boundary — conversions go through TickRate, and
/// the view layer's wall-clock dt is turned into ticks by the game loop.

/// A duration in ticks. Signed, so `a - b` is always meaningful.
class TickSpan {
  public:
    using Rep = std::int64_t;

    constexpr TickSpan() = default;
    constexpr explicit TickSpan(Rep ticks) : ticks_(ticks) {}

    [[nodiscard]] constexpr Rep count() const { return ticks_; }

    [[nodiscard]] constexpr auto operator<=>(const TickSpan&) const = default;
    [[nodiscard]] constexpr bool operator==(const TickSpan&) const = default;

    [[nodiscard]] constexpr TickSpan operator+(TickSpan other) const {
        return TickSpan{ticks_ + other.ticks_};
    }
    [[nodiscard]] constexpr TickSpan operator-(TickSpan other) const {
        return TickSpan{ticks_ - other.ticks_};
    }
    [[nodiscard]] constexpr TickSpan operator*(Rep factor) const {
        return TickSpan{ticks_ * factor};
    }
    [[nodiscard]] constexpr TickSpan operator-() const { return TickSpan{-ticks_}; }

    constexpr TickSpan& operator+=(TickSpan other) {
        ticks_ += other.ticks_;
        return *this;
    }

  private:
    Rep ticks_ = 0;
};

/// A point in simulation time: ticks elapsed since the simulation started.
class Tick {
  public:
    using Rep = std::int64_t;

    constexpr Tick() = default;
    constexpr explicit Tick(Rep value) : value_(value) {}

    [[nodiscard]] constexpr Rep value() const { return value_; }

    [[nodiscard]] constexpr auto operator<=>(const Tick&) const = default;
    [[nodiscard]] constexpr bool operator==(const Tick&) const = default;

    [[nodiscard]] constexpr Tick operator+(TickSpan span) const {
        return Tick{value_ + span.count()};
    }
    [[nodiscard]] constexpr Tick operator-(TickSpan span) const {
        return Tick{value_ - span.count()};
    }
    /// Distance between two points in time.
    [[nodiscard]] constexpr TickSpan operator-(Tick other) const {
        return TickSpan{value_ - other.value_};
    }

    constexpr Tick& operator+=(TickSpan span) {
        value_ += span.count();
        return *this;
    }
    constexpr Tick& operator++() {
        ++value_;
        return *this;
    }

  private:
    Rep value_ = 0;
};

/// The tick that a fresh simulation starts on.
inline constexpr Tick kSimulationStart{0};

/// Converts between ticks and seconds. This is the only place in the core that
/// knows how long a tick lasts; everything else speaks Tick / TickSpan.
class TickRate {
  public:
    /// `ticks_per_second` must be > 0; values <= 0 are clamped to 1 so a
    /// misconfigured rate degrades instead of dividing by zero.
    constexpr explicit TickRate(int ticks_per_second)
        : per_second_(ticks_per_second > 0 ? ticks_per_second : 1) {}

    [[nodiscard]] constexpr int per_second() const { return per_second_; }

    /// Length of one tick in seconds — the fixed dt of the simulation.
    [[nodiscard]] constexpr double seconds_per_tick() const { return 1.0 / per_second_; }

    /// Rounds `seconds` to the nearest whole number of ticks. Durations from
    /// game data ("3 second buff") enter the simulation through here.
    [[nodiscard]] TickSpan ticks_from_seconds(double seconds) const {
        return TickSpan{static_cast<TickSpan::Rep>(std::llround(seconds * per_second_))};
    }

    /// Ticks back to seconds, for display and for per-second stats.
    [[nodiscard]] constexpr double seconds_from_ticks(TickSpan span) const {
        return static_cast<double>(span.count()) / per_second_;
    }

    [[nodiscard]] constexpr double seconds_at(Tick tick) const {
        return static_cast<double>(tick.value()) / per_second_;
    }

  private:
    int per_second_;
};

/// The default simulation rate. Matches the view layer's default so the demo
/// and the headless simulation agree on what a tick means.
inline constexpr TickRate kDefaultTickRate{60};

} // namespace moba_sim
