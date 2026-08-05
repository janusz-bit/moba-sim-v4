#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "sim/tick.hpp"

using namespace moba_sim;

TEST_CASE("Tick arithmetic keeps points and durations distinct", "[tick]") {
    const Tick start{100};
    const TickSpan three_ticks{3};

    // point + duration = point
    REQUIRE((start + three_ticks) == Tick{103});
    REQUIRE((start - three_ticks) == Tick{97});
    // point - point = duration
    REQUIRE((Tick{103} - start) == TickSpan{3});
}

TEST_CASE("Tick comparisons order simulation time", "[tick]") {
    REQUIRE(Tick{5} < Tick{6});
    REQUIRE(Tick{6} > Tick{5});
    REQUIRE(Tick{5} == Tick{5});
    REQUIRE(Tick{5} <= Tick{5});
}

TEST_CASE("TickSpan supports signed arithmetic", "[tick]") {
    REQUIRE((TickSpan{10} + TickSpan{5}) == TickSpan{15});
    REQUIRE((TickSpan{10} - TickSpan{15}) == TickSpan{-5});
    REQUIRE((TickSpan{10} * 3) == TickSpan{30});
    REQUIRE((-TickSpan{10}) == TickSpan{-10});

    TickSpan span{4};
    span += TickSpan{6};
    REQUIRE(span == TickSpan{10});
}

TEST_CASE("TickRate converts seconds to whole ticks", "[tick]") {
    const TickRate rate{60};

    REQUIRE(rate.per_second() == 60);
    REQUIRE(rate.ticks_from_seconds(1.0) == TickSpan{60});
    REQUIRE(rate.ticks_from_seconds(3.0) == TickSpan{180});
    REQUIRE(rate.ticks_from_seconds(0.5) == TickSpan{30});
    // 2.5s at 60/s = 150 ticks exactly
    REQUIRE(rate.ticks_from_seconds(2.5) == TickSpan{150});
}

TEST_CASE("TickRate rounds durations that do not land on a tick", "[tick]") {
    const TickRate rate{60};

    // 1/100 s is 0.6 ticks -> 1 tick. Rounding (not truncation) keeps short
    // durations from collapsing to zero and vanishing.
    REQUIRE(rate.ticks_from_seconds(0.01) == TickSpan{1});
    REQUIRE(rate.ticks_from_seconds(0.001) == TickSpan{0});
}

TEST_CASE("TickRate converts ticks back to seconds", "[tick]") {
    const TickRate rate{60};

    REQUIRE_THAT(rate.seconds_per_tick(), Catch::Matchers::WithinAbs(1.0 / 60.0, 1e-12));
    REQUIRE_THAT(rate.seconds_from_ticks(TickSpan{90}), Catch::Matchers::WithinAbs(1.5, 1e-12));
    REQUIRE_THAT(rate.seconds_at(Tick{120}), Catch::Matchers::WithinAbs(2.0, 1e-12));
}

TEST_CASE("TickRate clamps a non-positive rate instead of dividing by zero", "[tick]") {
    REQUIRE(TickRate{0}.per_second() == 1);
    REQUIRE(TickRate{-30}.per_second() == 1);
}

TEST_CASE("Integer ticks make a duration exact at any rate", "[tick]") {
    // The reason time is integral: accumulating 1/60 as a double 180 times
    // does not land on 3.0, so `now >= expiry` would fire a tick early or late
    // depending on the rate. Ticks make the same duration exact everywhere.
    for (const int per_second : {10, 30, 60, 128}) {
        const TickRate rate{per_second};
        const TickSpan three_seconds = rate.ticks_from_seconds(3.0);
        REQUIRE(three_seconds.count() == 3 * per_second);
        REQUIRE_THAT(rate.seconds_from_ticks(three_seconds),
                     Catch::Matchers::WithinAbs(3.0, 1e-12));
    }
}

TEST_CASE("Ticks are constexpr-usable", "[tick]") {
    // Durations from game data can be compile-time constants.
    constexpr Tick start{0};
    constexpr TickSpan span{180};
    constexpr Tick expiry = start + span;
    static_assert(expiry.value() == 180);
    static_assert(kSimulationStart.value() == 0);
    static_assert(kDefaultTickRate.per_second() == 60);
    REQUIRE(expiry == Tick{180});
}
