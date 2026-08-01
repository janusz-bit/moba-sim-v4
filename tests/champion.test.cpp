#include <catch2/catch_test_macros.hpp>

#include "champions/champion.hpp"

using namespace moba_sim;

TEST_CASE("Champion holds base stats in the LoL wiki format", "[champion]") {
    // https://wiki.leagueoflegends.com/en-us/Ahri — "Base statistics"
    const Champion ahri{
        .name = "Ahri",
        .role = Role::Mage,
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
    REQUIRE(ahri.role == Role::Mage);
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
    REQUIRE(champion.role == Role::Fighter);
    REQUIRE(champion.resource_type == ResourceType::Mana);
    REQUIRE(champion.range_type == RangeType::Melee);
}
