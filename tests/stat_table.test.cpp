#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "stats/stat_id.hpp"
#include "stats/stat_mask.hpp"
#include "stats/stat_table.hpp"

using namespace moba_sim;

TEST_CASE("kAllStats is derived from the enum and excludes the sentinel", "[stats]") {
    // Deriving the list from StatId::Count is what keeps it from drifting: a
    // new stat cannot be missing from kAllStats, so no table indexed by
    // stat_index() can be sized wrong.
    REQUIRE(kAllStats.size() == kStatCount);
    REQUIRE(kStatCount == static_cast<std::size_t>(StatId::Count));

    for (std::size_t i = 0; i < kAllStats.size(); ++i) {
        REQUIRE(stat_index(kAllStats[i]) == i);
        REQUIRE(kAllStats[i] != StatId::Count);
    }
}

TEST_CASE("Every stat has a name", "[stats]") {
    for (StatId stat : kAllStats) {
        REQUIRE_FALSE(stat_name(stat).empty());
    }
    REQUIRE(stat_name(StatId::AttackDamage) == "AttackDamage");
    REQUIRE(stat_name(StatId::MovementSpeed) == "MovementSpeed");
}

TEST_CASE("StatMask tracks membership", "[stats]") {
    const StatMask mask{StatId::AttackDamage, StatId::Armor};

    REQUIRE(mask.contains(StatId::AttackDamage));
    REQUIRE(mask.contains(StatId::Armor));
    REQUIRE_FALSE(mask.contains(StatId::Health));
    REQUIRE(mask.size() == 2);
    REQUIRE_FALSE(mask.empty());
}

TEST_CASE("An empty StatMask contains nothing", "[stats]") {
    const StatMask mask;

    REQUIRE(mask.empty());
    REQUIRE(mask.size() == 0);
    for (StatId stat : kAllStats) {
        REQUIRE_FALSE(mask.contains(stat));
    }
}

TEST_CASE("StatMask::all contains every stat", "[stats]") {
    const StatMask mask = StatMask::all();

    REQUIRE(mask.size() == kStatCount);
    for (StatId stat : kAllStats) {
        REQUIRE(mask.contains(stat));
    }
}

TEST_CASE("StatMask intersection detects shared stats", "[stats]") {
    const StatMask ad{StatId::AttackDamage};
    const StatMask armor{StatId::Armor};
    const StatMask both{StatId::AttackDamage, StatId::Armor};

    REQUIRE_FALSE(ad.intersects(armor));
    REQUIRE(ad.intersects(both));
    REQUIRE(both.intersects(armor));
    REQUIRE_FALSE(ad.intersects(StatMask{}));
}

TEST_CASE("StatMask union combines masks", "[stats]") {
    const StatMask combined = StatMask{StatId::Health} | StatMask{StatId::Armor};

    REQUIRE(combined.contains(StatId::Health));
    REQUIRE(combined.contains(StatId::Armor));
    REQUIRE(combined.size() == 2);

    StatMask accumulated{StatId::Health};
    accumulated |= StatMask{StatId::Armor};
    REQUIRE(accumulated == combined);
}

TEST_CASE("StatTable computes each stat independently", "[stats]") {
    StatTable table;
    table[StatId::Health].add_base(590, "base");
    table[StatId::AttackDamage].add_base(53, "base");
    table[StatId::AttackDamage].add_inc(0.1, "buff");

    REQUIRE(table.compute(StatId::Health) == 590);
    REQUIRE_THAT(table.compute(StatId::AttackDamage), Catch::Matchers::WithinAbs(58.3, 1e-9));
    REQUIRE(table.compute(StatId::Armor) == 0);
}

TEST_CASE("StatTable applies modifiers through one entry point", "[stats]") {
    StatTable table;
    table.apply(base_mod(StatId::Health, 590, "base"));
    table.apply(base_mod(StatId::Health, 150, "Ruby Crystal"));
    table.apply(inc_mod(StatId::Health, 0.1, "buff"));

    REQUIRE_THAT(table.compute(StatId::Health), Catch::Matchers::WithinAbs(814.0, 1e-9));

    const StatBreakdown breakdown = table.breakdown(StatId::Health);
    REQUIRE(breakdown.base.size() == 2);
    REQUIRE(breakdown.base[1].source == "Ruby Crystal");
    REQUIRE(breakdown.inc.size() == 1);
}

TEST_CASE("StatTable apply can override the source label", "[stats]") {
    StatTable table;
    table.apply(base_mod(StatId::Health, 150), "Ruby Crystal");

    REQUIRE(table.breakdown(StatId::Health).base[0].source == "Ruby Crystal");
}

TEST_CASE("StatTable clear resets every stat", "[stats]") {
    StatTable table;
    for (StatId stat : kAllStats) {
        table[stat].add_base(100, "base");
    }

    table.clear();

    for (StatId stat : kAllStats) {
        REQUIRE(table.compute(stat) == 0);
    }
}

TEST_CASE("Modifier helpers build the right bucket", "[stats]") {
    const Modifier base = base_mod(StatId::Health, 150, "Ruby");
    const Modifier inc = inc_mod(StatId::Health, 0.1, "Zeal");
    const Modifier more = more_mod(StatId::Health, 0.2, "IE");

    REQUIRE(base.kind == ModifierKind::Base);
    REQUIRE(base.stat == StatId::Health);
    REQUIRE(base.value == 150);
    REQUIRE(base.source == "Ruby");
    REQUIRE(inc.kind == ModifierKind::Inc);
    REQUIRE(more.kind == ModifierKind::More);
}

TEST_CASE("apply_modifier routes every kind to its bucket", "[stats]") {
    // One shared routing function means items, effects and base stats cannot
    // disagree about what ModifierKind means.
    StatPipeline pipe;
    apply_modifier(pipe, base_mod(StatId::Health, 100, "a"));
    apply_modifier(pipe, inc_mod(StatId::Health, 0.5, "b"));
    apply_modifier(pipe, more_mod(StatId::Health, 0.5, "c"));

    REQUIRE(pipe.base_total() == 100);
    REQUIRE(pipe.inc_multiplier() == 1.5);
    REQUIRE(pipe.more_multiplier() == 1.5);
    REQUIRE(pipe.compute() == 225);
}
