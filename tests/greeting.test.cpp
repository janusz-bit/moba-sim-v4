#include <catch2/catch_test_macros.hpp>

#include "core/greeting.hpp"

TEST_CASE("greeting returns a personalized message", "[greeting]") {
    REQUIRE(moba_sim::greeting("World") == "Hello, World!");
    REQUIRE(moba_sim::greeting("moba-sim") == "Hello, moba-sim!");
}
