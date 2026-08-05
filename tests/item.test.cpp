#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "champions/champion.hpp"
#include "items/item.hpp"

using namespace moba_sim;

TEST_CASE("Item with no modifiers does nothing", "[item]") {
    const ChampionData data{.name = "Ahri", .health = 590};
    Champion champ(data);

    champ.equip(Item{.name = "Empty", .modifiers = {}});

    REQUIRE(champ.compute(StatId::Health) == 590);
}

TEST_CASE("Item applies a single Base modifier", "[item]") {
    const ChampionData data{.name = "Ahri", .health = 590};
    Champion champ(data);

    champ.equip(Item{.name = "Ruby Crystal", .modifiers = {base_mod(StatId::Health, 150)}});

    REQUIRE(champ.compute(StatId::Health) == 740);
}

TEST_CASE("Item applies mixed modifiers across stats", "[item]") {
    const ChampionData data{.name = "Ahri", .attack_damage = 53, .armor = 21};
    Champion champ(data);

    champ.equip(Item{
        .name = "Chain Vest",
        .modifiers = {base_mod(StatId::Armor, 40), inc_mod(StatId::AttackDamage, 0.1)},
    });

    // Armor: 21 + 40 = 61
    REQUIRE(champ.compute(StatId::Armor) == 61);
    // AD: 53 * (1 + 0.1) = 58.3
    REQUIRE_THAT(champ.compute(StatId::AttackDamage), Catch::Matchers::WithinAbs(58.3, 1e-9));
}

TEST_CASE("Multiple items stack on the same stat", "[item]") {
    const ChampionData data{.name = "Ahri", .health = 590};
    Champion champ(data);

    champ.equip(Item{.name = "Ruby Crystal", .modifiers = {base_mod(StatId::Health, 150)}});
    champ.equip(Item{.name = "Giant's Belt", .modifiers = {base_mod(StatId::Health, 380)}});

    // 590 + 150 + 380 = 1120
    REQUIRE(champ.compute(StatId::Health) == 1120);
}

TEST_CASE("Item applies More modifier multiplicatively", "[item]") {
    const ChampionData data{.name = "Ahri", .attack_damage = 100};
    Champion champ(data);

    champ.equip(Item{
        .name = "More AD",
        .modifiers = {base_mod(StatId::AttackDamage, 50), more_mod(StatId::AttackDamage, 0.1)},
    });

    // (100 + 50) * 1.1 = 165
    REQUIRE(champ.compute(StatId::AttackDamage) == 165);
}

TEST_CASE("Item modifiers are labeled with the item name", "[item]") {
    const ChampionData data{.name = "Ahri", .attack_damage = 100};
    Champion champ(data);

    champ.equip(Item{.name = "B.F. Sword", .modifiers = {base_mod(StatId::AttackDamage, 40)}});

    const StatBreakdown breakdown = champ.explain(StatId::AttackDamage);
    REQUIRE(breakdown.base.size() == 2);
    CHECK(breakdown.base[1].source == "B.F. Sword");
}

TEST_CASE("Item modifier keeps its own source label when set", "[item]") {
    // A single item can attribute lines to sub-sources, e.g. an item passive.
    const ChampionData data{.name = "Ahri", .attack_damage = 100};
    Champion champ(data);

    champ.equip(Item{
        .name = "Zeal",
        .modifiers = {base_mod(StatId::AttackDamage, 10, "Zeal (passive)")},
    });

    const StatBreakdown breakdown = champ.explain(StatId::AttackDamage);
    REQUIRE(breakdown.base.size() == 2);
    CHECK(breakdown.base[1].source == "Zeal (passive)");
}

TEST_CASE("Item modifiers_for returns only matching stat", "[item]") {
    const Item item{
        .name = "Mixed",
        .modifiers =
            {
                base_mod(StatId::Health, 150),
                base_mod(StatId::AttackDamage, 20),
                inc_mod(StatId::AttackDamage, 0.1),
                base_mod(StatId::Armor, 10),
            },
    };

    const auto ad_mods = item.modifiers_for(StatId::AttackDamage);
    REQUIRE(ad_mods.size() == 2);
    REQUIRE(ad_mods[0].kind == ModifierKind::Base);
    REQUIRE(ad_mods[0].value == 20);
    REQUIRE(ad_mods[1].kind == ModifierKind::Inc);
    REQUIRE(ad_mods[1].value == 0.1);

    REQUIRE(item.modifiers_for(StatId::Health).size() == 1);
    REQUIRE(item.modifiers_for(StatId::MagicResist).empty());
}
