#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include "champions/champion.hpp"
#include "stats/stat_breakdown.hpp"
#include "stats/stat_pipeline.hpp"

using namespace moba_sim;
using Catch::Matchers::ContainsSubstring;

TEST_CASE("Pipeline breakdown labels every modifier with its source", "[stats]") {
    StatPipeline pipeline;
    pipeline.add_base(59, "Ahri base, lvl 3");
    pipeline.add_base(40, "B.F. Sword");
    pipeline.add_inc(0.2, "Zeal");
    pipeline.add_more(0.1, "Infinity Edge");

    const StatBreakdown breakdown = pipeline.breakdown();

    REQUIRE(breakdown.base.size() == 2);
    CHECK(breakdown.base[0].value == 59);
    CHECK(breakdown.base[0].source == "Ahri base, lvl 3");
    CHECK(breakdown.base[1].value == 40);
    CHECK(breakdown.base[1].source == "B.F. Sword");

    REQUIRE(breakdown.inc.size() == 1);
    CHECK(breakdown.inc[0].source == "Zeal");

    REQUIRE(breakdown.more.size() == 1);
    CHECK(breakdown.more[0].source == "Infinity Edge");

    CHECK(breakdown.base_total == 99.0);
    CHECK(breakdown.inc_multiplier == 1.2);
    CHECK_THAT(breakdown.more_multiplier, Catch::Matchers::WithinAbs(1.1, 1e-12));
    CHECK_THAT(breakdown.total, Catch::Matchers::WithinAbs(99 * 1.2 * 1.1, 1e-9));
    CHECK(breakdown.total == pipeline.compute());
}

TEST_CASE("Unlabeled modifiers are reported as unknown", "[stats]") {
    StatPipeline pipeline;
    pipeline.add_base(10);

    const StatBreakdown breakdown = pipeline.breakdown();

    REQUIRE(breakdown.base.size() == 1);
    CHECK(breakdown.base[0].source.empty());

    const std::string report = format_breakdown("Health", breakdown);
    CHECK_THAT(report, ContainsSubstring("(unknown)"));
}

TEST_CASE("format_breakdown shows the whole derivation", "[stats]") {
    StatPipeline pipeline;
    pipeline.add_base(59, "Ahri base, lvl 3");
    pipeline.add_base(40, "B.F. Sword");
    pipeline.add_inc(0.2, "Zeal");

    const std::string report = format_breakdown("AttackDamage", pipeline.breakdown());

    CHECK_THAT(report, ContainsSubstring("AttackDamage = 118.8"));
    CHECK_THAT(report, ContainsSubstring("Base = 99"));
    CHECK_THAT(report, ContainsSubstring("59"));
    CHECK_THAT(report, ContainsSubstring("Ahri base, lvl 3"));
    CHECK_THAT(report, ContainsSubstring("B.F. Sword"));
    CHECK_THAT(report, ContainsSubstring("Inc = 1.2"));
    CHECK_THAT(report, ContainsSubstring("Zeal"));
    CHECK_THAT(report, ContainsSubstring("99 * 1.2 * 1 = 118.8"));
}

TEST_CASE("Champion explain traces base stats and items", "[champion]") {
    const ChampionData ahri{.name = "Ahri", .attack_damage = 53, .attack_damage_growth = 3};
    Champion champ(ahri, 3);
    champ.equip(Item{.name = "B.F. Sword", .modifiers = {base_mod(StatId::AttackDamage, 40)}});

    const StatBreakdown breakdown = champ.explain(StatId::AttackDamage);

    // Base: 53 + 3 * 2 = 59 from champion, +40 from the item.
    REQUIRE(breakdown.base.size() == 2);
    CHECK(breakdown.base[0].value == 59);
    CHECK(breakdown.base[0].source == "Ahri base, lvl 3");
    CHECK(breakdown.base[1].value == 40);
    CHECK(breakdown.base[1].source == "B.F. Sword");
    CHECK(breakdown.total == champ.compute(StatId::AttackDamage));

    const std::string report = format_breakdown("AttackDamage", breakdown);
    CHECK_THAT(report, ContainsSubstring("Ahri base, lvl 3"));
    CHECK_THAT(report, ContainsSubstring("B.F. Sword"));
    CHECK_THAT(report, ContainsSubstring("AttackDamage = 99"));
}

TEST_CASE("Champion explain traces effects alongside items", "[champion]") {
    // Provenance has to survive the effect layer: an effect's contribution is
    // labeled with its EffectKey, so "why is my AD 217.8?" stays answerable.
    const ChampionData ahri{.name = "Ahri", .attack_damage = 53, .attack_damage_growth = 3};
    Champion champ(ahri, 3);
    champ.equip(Item{.name = "B.F. Sword", .modifiers = {base_mod(StatId::AttackDamage, 40)}});
    champ.apply_effect(flat_effect({.source = "Baron", .name = "Hand of Baron"},
                                   {base_mod(StatId::AttackDamage, 20)}));
    champ.apply_effect(
        flat_effect({.source = "Ahri", .name = "Fury"}, {inc_mod(StatId::AttackDamage, 0.1)}));

    const StatBreakdown breakdown = champ.explain(StatId::AttackDamage);

    // Base: 59 champion + 40 item + 20 buff = 119; Inc: 1.1 -> 130.9
    REQUIRE(breakdown.base.size() == 3);
    CHECK(breakdown.base[0].source == "Ahri base, lvl 3");
    CHECK(breakdown.base[1].source == "B.F. Sword");
    CHECK(breakdown.base[2].source == "Baron (Hand of Baron)");
    REQUIRE(breakdown.inc.size() == 1);
    CHECK(breakdown.inc[0].source == "Ahri (Fury)");
    CHECK_THAT(breakdown.total, Catch::Matchers::WithinAbs(130.9, 1e-9));

    const std::string report = format_breakdown("AttackDamage", breakdown);
    CHECK_THAT(report, ContainsSubstring("Baron (Hand of Baron)"));
    CHECK_THAT(report, ContainsSubstring("Ahri (Fury)"));
}

TEST_CASE("Stacked effects report their stack count in the breakdown", "[champion]") {
    const ChampionData data{.name = "Nasus", .attack_damage = 100};
    Champion champ(data);
    const Effect siphon{
        .key = {.source = "Nasus", .name = "Siphoning Strike"},
        .writes = {StatId::AttackDamage},
        .policy = StackPolicy::Stack,
        .max_stacks = 10,
        .contribute = [](const EffectContext&,
                         ModifierSink& sink) { sink.add_base(StatId::AttackDamage, 3); },
    };

    champ.apply_effect(siphon);
    champ.apply_effect(siphon);
    champ.apply_effect(siphon);

    const StatBreakdown breakdown = champ.explain(StatId::AttackDamage);
    REQUIRE(breakdown.base.size() == 2);
    CHECK(breakdown.base[1].source == "Nasus (Siphoning Strike) x3");
    CHECK(breakdown.base[1].value == 9.0);
}
