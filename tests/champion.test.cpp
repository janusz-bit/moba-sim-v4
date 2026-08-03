#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "champions/champion.hpp"

using namespace moba_sim;

TEST_CASE("ChampionData holds base stats in the LoL wiki format", "[champion]") {
    // https://wiki.leagueoflegends.com/en-us/Ahri — "Base statistics"
    const ChampionData ahri{
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

TEST_CASE("ChampionData has sensible defaults", "[champion]") {
    const ChampionData data;

    REQUIRE(data.name.empty());
    REQUIRE(data.health == 0.0);
    REQUIRE(data.resource_type == ResourceType::Mana);
    REQUIRE(data.range_type == RangeType::Melee);
}

TEST_CASE("ChampionData base_value uses base at level 1", "[champion]") {
    const ChampionData ahri{
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

TEST_CASE("ChampionData base_value adds growth per level", "[champion]") {
    const ChampionData ahri{
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
    const ChampionData champ{
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

TEST_CASE("Champion seeds each stat pipeline from ChampionData", "[champion]") {
    const ChampionData ahri{.name = "Ahri", .health = 590, .health_growth = 104};

    const Champion champ(ahri);
    REQUIRE(champ.compute(StatId::Health) == 590);
    REQUIRE(champ.pipeline(StatId::Health).base_total() == 590);
}

TEST_CASE("Champion seeds pipelines at the given level", "[champion]") {
    const ChampionData ahri{.name = "Ahri", .health = 590, .health_growth = 104};

    // Level 5: 590 + 104 * 4 = 1006
    const Champion champ(ahri, 5);
    REQUIRE(champ.compute(StatId::Health) == 1006);
}

TEST_CASE("Champion accepts Inc/More modifiers on top of seeded base", "[champion]") {
    const ChampionData ahri{.name = "Ahri", .attack_damage = 53, .attack_damage_growth = 3};

    // Level 6 AD: 53 + 3 * 5 = 68. Add +10 AD (Base), +20% Inc, +10% More.
    Champion champ(ahri, 6);
    champ.pipeline(StatId::AttackDamage).add_base(10);
    champ.pipeline(StatId::AttackDamage).add_inc(0.2);
    champ.pipeline(StatId::AttackDamage).add_more(0.1);

    // (68 + 10) * (1 + 0.2) * (1 + 0.1) = 78 * 1.2 * 1.1 = 102.96
    REQUIRE_THAT(champ.compute(StatId::AttackDamage), Catch::Matchers::WithinAbs(102.96, 1e-9));
}

TEST_CASE("Champion equip applies item modifiers", "[champion]") {
    const ChampionData ahri{.name = "Ahri", .health = 590};
    Champion champ(ahri);
    const Item ruby{.name = "Ruby Crystal",
                    .modifiers = {{StatId::Health, ModifierKind::Base, 150}}};

    champ.equip(ruby);

    REQUIRE(champ.items().size() == 1);
    REQUIRE(champ.items()[0].name == "Ruby Crystal");
    // 590 + 150 = 740
    REQUIRE(champ.compute(StatId::Health) == 740);
}

TEST_CASE("Champion equip stacks multiple items", "[champion]") {
    const ChampionData ahri{.name = "Ahri", .health = 590};
    Champion champ(ahri);
    const Item ruby{.name = "Ruby Crystal",
                    .modifiers = {{StatId::Health, ModifierKind::Base, 150}}};
    const Item belt{.name = "Giant's Belt",
                    .modifiers = {{StatId::Health, ModifierKind::Base, 380}}};

    champ.equip(ruby);
    champ.equip(belt);

    REQUIRE(champ.items().size() == 2);
    // 590 + 150 + 380 = 1120
    REQUIRE(champ.compute(StatId::Health) == 1120);
}

TEST_CASE("Champion unequip removes item and rebuilds pipelines", "[champion]") {
    const ChampionData ahri{.name = "Ahri", .health = 590};
    Champion champ(ahri);
    const Item ruby{.name = "Ruby Crystal",
                    .modifiers = {{StatId::Health, ModifierKind::Base, 150}}};
    const Item belt{.name = "Giant's Belt",
                    .modifiers = {{StatId::Health, ModifierKind::Base, 380}}};

    champ.equip(ruby);
    champ.equip(belt);
    REQUIRE(champ.compute(StatId::Health) == 1120);

    champ.unequip(ruby);

    REQUIRE(champ.items().size() == 1);
    REQUIRE(champ.items()[0].name == "Giant's Belt");
    // 590 + 380 = 970
    REQUIRE(champ.compute(StatId::Health) == 970);
}

TEST_CASE("Champion unequip of absent item is a no-op", "[champion]") {
    const ChampionData ahri{.name = "Ahri", .health = 590};
    Champion champ(ahri);
    const Item ruby{.name = "Ruby Crystal",
                    .modifiers = {{StatId::Health, ModifierKind::Base, 150}}};
    const Item other{.name = "Phantom", .modifiers = {}};

    champ.equip(ruby);
    champ.unequip(other);

    REQUIRE(champ.items().size() == 1);
    REQUIRE(champ.compute(StatId::Health) == 740);
}

TEST_CASE("Champion starts with no items", "[champion]") {
    const ChampionData ahri{.name = "Ahri", .health = 590};
    const Champion champ(ahri);

    REQUIRE(champ.items().empty());
}