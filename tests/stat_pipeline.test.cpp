#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "stats/stat_pipeline.hpp"

using namespace moba_sim;

TEST_CASE("Base modifiers add up", "[stats]") {
    StatPipeline pipeline;
    pipeline.add({ModifierKind::Base, 10});
    pipeline.add({ModifierKind::Base, 20});
    pipeline.add({ModifierKind::Base, 30});

    // 10 + 20 + 30 = 60
    REQUIRE(pipeline.base_total() == 60.0);
    REQUIRE(pipeline.compute() == 60.0);
}

TEST_CASE("Inc modifiers sum from 1.0", "[stats]") {
    StatPipeline pipeline;
    pipeline.add({ModifierKind::Base, 60});
    pipeline.add({ModifierKind::Inc, 0.1});
    pipeline.add({ModifierKind::Inc, 0.2});

    // 1.0 + 0.1 + 0.2 = 1.3
    REQUIRE(pipeline.inc_multiplier() == 1.3);
    REQUIRE(pipeline.compute() == 78.0);
}

TEST_CASE("More modifiers multiply", "[stats]") {
    StatPipeline pipeline;
    pipeline.add({ModifierKind::Base, 60});
    pipeline.add({ModifierKind::More, 0.1});
    pipeline.add({ModifierKind::More, 0.2});
    pipeline.add({ModifierKind::More, 0.3});

    // 1.1 * 1.2 * 1.3 = 1.716
    REQUIRE_THAT(pipeline.more_multiplier(), Catch::Matchers::WithinAbs(1.716, 1e-9));
    REQUIRE_THAT(pipeline.compute(), Catch::Matchers::WithinAbs(102.96, 1e-9));
}

TEST_CASE("Full pipeline matches Base * (1 + Inc) * More", "[stats]") {
    StatPipeline pipeline;
    pipeline.add({ModifierKind::Base, 10});
    pipeline.add({ModifierKind::Base, 20});
    pipeline.add({ModifierKind::Base, 30});
    pipeline.add({ModifierKind::Inc, 0.1});
    pipeline.add({ModifierKind::Inc, 0.2});
    pipeline.add({ModifierKind::More, 0.1});
    pipeline.add({ModifierKind::More, 0.2});
    pipeline.add({ModifierKind::More, 0.3});

    // 60 * 1.3 * 1.716 = 133.848
    REQUIRE_THAT(pipeline.compute(), Catch::Matchers::WithinAbs(133.848, 1e-9));
}

TEST_CASE("Empty pipeline produces zero", "[stats]") {
    StatPipeline pipeline;

    REQUIRE(pipeline.base_total() == 0.0);
    REQUIRE(pipeline.inc_multiplier() == 1.0);
    REQUIRE(pipeline.more_multiplier() == 1.0);
    REQUIRE(pipeline.compute() == 0.0);
}

TEST_CASE("Inc/More without base produce zero", "[stats]") {
    StatPipeline pipeline;
    pipeline.add({ModifierKind::Inc, 0.5});
    pipeline.add({ModifierKind::More, 0.5});

    REQUIRE(pipeline.compute() == 0.0);
}