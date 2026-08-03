#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "champions/champion.hpp"
#include "items/item.hpp"

using namespace moba_sim;

namespace {

// Applies every modifier of `item` to the matching pipeline of `champ`.
void apply_item(const Item& item, Champion& champ) {
    for (const auto& mod : item.modifiers) {
        switch (mod.kind) {
        case ModifierKind::Base:
            champ.pipeline(mod.stat).add_base(mod.value);
            break;
        case ModifierKind::Inc:
            champ.pipeline(mod.stat).add_inc(mod.value);
            break;
        case ModifierKind::More:
            champ.pipeline(mod.stat).add_more(mod.value);
            break;
        }
    }
}

} // namespace

TEST_CASE("Item with no modifiers does nothing", "[item]") {
    const ChampionData data{.name = "Ahri", .health = 590};
    Champion champ(data);
    const Item item{.name = "Empty", .modifiers = {}};

    apply_item(item, champ);

    REQUIRE(champ.compute(StatId::Health) == 590);
}

TEST_CASE("Item applies a single Base modifier", "[item]") {
    const ChampionData data{.name = "Ahri", .health = 590};
    Champion champ(data);
    const Item item{
        .name = "Ruby Crystal",
        .modifiers = {{StatId::Health, ModifierKind::Base, 150}},
    };

    apply_item(item, champ);

    REQUIRE(champ.compute(StatId::Health) == 740);
}

TEST_CASE("Item applies mixed modifiers across stats", "[item]") {
    const ChampionData data{.name = "Ahri", .attack_damage = 53, .armor = 21};
    Champion champ(data);
    const Item item{
        .name = "Chain Vest",
        .modifiers =
            {
                {StatId::Armor, ModifierKind::Base, 40},
                {StatId::AttackDamage, ModifierKind::Inc, 0.1},
            },
    };

    apply_item(item, champ);

    // Armor: 21 + 40 = 61
    REQUIRE(champ.compute(StatId::Armor) == 61);
    // AD: 53 * (1 + 0.1) = 58.3
    REQUIRE_THAT(champ.compute(StatId::AttackDamage), Catch::Matchers::WithinAbs(58.3, 1e-9));
}

TEST_CASE("Multiple items stack on the same stat", "[item]") {
    const ChampionData data{.name = "Ahri", .health = 590};
    Champion champ(data);
    const Item ruby{
        .name = "Ruby Crystal",
        .modifiers = {{StatId::Health, ModifierKind::Base, 150}},
    };
    const Item more_hp{
        .name = "Giant's Belt",
        .modifiers = {{StatId::Health, ModifierKind::Base, 380}},
    };

    apply_item(ruby, champ);
    apply_item(more_hp, champ);

    // 590 + 150 + 380 = 1120
    REQUIRE(champ.compute(StatId::Health) == 1120);
}

TEST_CASE("Item applies More modifier multiplicatively", "[item]") {
    const ChampionData data{.name = "Ahri", .attack_damage = 100};
    Champion champ(data);
    const Item item{
        .name = "More AD",
        .modifiers =
            {
                {StatId::AttackDamage, ModifierKind::Base, 50},
                {StatId::AttackDamage, ModifierKind::More, 0.1},
            },
    };

    apply_item(item, champ);

    // (100 + 50) * 1.1 = 165
    REQUIRE(champ.compute(StatId::AttackDamage) == 165);
}

TEST_CASE("Item modifiers_for returns only matching stat", "[item]") {
    const Item item{
        .name = "Mixed",
        .modifiers =
            {
                {StatId::Health, ModifierKind::Base, 150},
                {StatId::AttackDamage, ModifierKind::Base, 20},
                {StatId::AttackDamage, ModifierKind::Inc, 0.1},
                {StatId::Armor, ModifierKind::Base, 10},
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