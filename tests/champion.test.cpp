#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "champions/champion.hpp"

using namespace moba_sim;

TEST_CASE("Champion holds base stats in the LoL wiki format", "[champion]") {
    // https://wiki.leagueoflegends.com/en-us/Ahri — "Base statistics"
    const Champion ahri{
        .name = "Ahri",
        .resource_type = ResourceType::Mana,
        .range_type = RangeType::Ranged,
        .health = 590,
        .health_regen = 2.5,
        .resource = 418,
        .resource_regen = 8,
        .attack_damage = 53,
        .attack_speed = 0.668,
        .armor = 21,
        .magic_resist = 30,
        .movement_speed = 330,
        .attack_range = 550,
        .health_growth = 104,
        .health_regen_growth = 0.6,
        .resource_growth = 25,
        .resource_regen_growth = 0.8,
        .attack_damage_growth = 3,
        .attack_speed_growth = 2.2,
        .armor_growth = 4.2,
        .magic_resist_growth = 1.3,
    };

    REQUIRE(ahri.name == "Ahri");
    REQUIRE(ahri.resource_type == ResourceType::Mana);
    REQUIRE(ahri.range_type == RangeType::Ranged);
    REQUIRE(ahri.health == 590);
    REQUIRE(ahri.attack_damage == 53);
    REQUIRE(ahri.attack_speed == 0.668);
    REQUIRE(ahri.movement_speed == 330);
    REQUIRE(ahri.attack_range == 550);
    REQUIRE(ahri.health_growth == 104);
    REQUIRE(ahri.magic_resist_growth == 1.3);
}

TEST_CASE("Champion has sensible defaults", "[champion]") {
    const Champion champion;

    REQUIRE(champion.name.empty());
    REQUIRE(champion.health == 0.0);
    REQUIRE(champion.resource_type == ResourceType::Mana);
    REQUIRE(champion.range_type == RangeType::Melee);
}

TEST_CASE("Champion base_value uses base at level 1", "[champion]") {
    const Champion ahri{
        .name = "Ahri",
        .range_type = RangeType::Ranged,
        .health = 590,
        .attack_damage = 53,
        .movement_speed = 330,
        .attack_range = 550,
        .health_growth = 104,
        .attack_damage_growth = 3,
    };

    REQUIRE(ahri.base_value(StatId::Health) == 590);
    REQUIRE(ahri.base_value(StatId::AttackDamage) == 53);
    REQUIRE(ahri.base_value(StatId::MovementSpeed) == 330);
    REQUIRE(ahri.base_value(StatId::AttackRange) == 550);
}

TEST_CASE("Champion base_value adds growth per level", "[champion]") {
    const Champion ahri{
        .name = "Ahri",
        .health = 590,
        .attack_damage = 53,
        .health_growth = 104,
        .attack_damage_growth = 3,
    };

    // Level 5: 590 + 104 * 4 = 1006
    REQUIRE(ahri.base_value(StatId::Health, 5) == 1006);
    // Level 18: 590 + 104 * 17 = 2358
    REQUIRE(ahri.base_value(StatId::Health, 18) == 2358);
    // Level 18 AD: 53 + 3 * 17 = 104
    REQUIRE(ahri.base_value(StatId::AttackDamage, 18) == 104);
}

TEST_CASE("Every stat applies growth, including MS and range", "[champion]") {
    const Champion champ{
        .name = "Champ",
        .movement_speed = 330,
        .attack_range = 550,
        .movement_speed_growth = 2,
        .attack_range_growth = 5,
    };

    // MS at level 6: 330 + 2 * 5 = 340
    REQUIRE(champ.base_value(StatId::MovementSpeed, 6) == 340);
    // Range at level 6: 550 + 5 * 5 = 575
    REQUIRE(champ.base_value(StatId::AttackRange, 6) == 575);
}

TEST_CASE("Champion pipeline_for seeds Base at level 1", "[champion]") {
    const Champion ahri{.name = "Ahri", .health = 590, .health_growth = 104};

    auto pipeline = ahri.pipeline_for(StatId::Health);
    REQUIRE(pipeline.base_total() == 590);
    REQUIRE(pipeline.compute() == 590);
}

TEST_CASE("Champion pipeline accepts Inc/More modifiers on top of base", "[champion]") {
    const Champion ahri{.name = "Ahri", .attack_damage = 53, .attack_damage_growth = 3};

    // Level 6 AD: 53 + 3 * 5 = 68. Add +10 AD (Base), +20% Inc, +10% More.
    auto pipeline = ahri.pipeline_for(StatId::AttackDamage, 6);
    pipeline.add({ModifierKind::Base, 10});
    pipeline.add({ModifierKind::Inc, 0.2});
    pipeline.add({ModifierKind::More, 0.1});

    // (68 + 10) * (1 + 0.2) * (1 + 0.1) = 78 * 1.2 * 1.1 = 102.96
    REQUIRE_THAT(pipeline.compute(), Catch::Matchers::WithinAbs(102.96, 1e-9));
}
