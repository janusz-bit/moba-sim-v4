#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "champions/champion.hpp"

using namespace moba_sim;

namespace {

ChampionData ahri_data() {
    // https://wiki.leagueoflegends.com/en-us/Ahri — "Base statistics"
    return ChampionData{
        .name = "Ahri",
        .resource_type = ResourceType::Mana,
        .range_type = RangeType::Ranged,
        .health = 590,
        .attack_damage = 53,
        .armor = 21,
        .movement_speed = 330,
        .health_growth = 104,
        .attack_damage_growth = 3,
    };
}

} // namespace

TEST_CASE("A timed buff on a champion expires on its own", "[champion][effects]") {
    Champion champ(ahri_data(), 6);
    const TickRate rate = champ.tick_rate();

    // Level 6 AD: 53 + 3 * 5 = 68. A 3 second +30 AD buff.
    REQUIRE(champ.compute(StatId::AttackDamage) == 68);

    const EffectKey key{.source = "Ahri", .name = "Fury"};
    champ.apply_effect(flat_effect(key, {base_mod(StatId::AttackDamage, 30)},
                                   Timed::for_span(champ.now(), rate.ticks_from_seconds(3.0))));

    REQUIRE(champ.compute(StatId::AttackDamage) == 98);

    // Still up one tick before the deadline (3s = 180 ticks at 60/s).
    champ.advance_to(Tick{179});
    REQUIRE(champ.compute(StatId::AttackDamage) == 98);
    REQUIRE(champ.effects().remaining_on(key, champ.now()) == TickSpan{1});

    const auto expired = champ.advance_to(Tick{180});
    REQUIRE(expired.size() == 1);
    REQUIRE(expired[0] == key);
    REQUIRE(champ.compute(StatId::AttackDamage) == 68);
}

TEST_CASE("Champion advance_by steps relative to the current time", "[champion][effects]") {
    Champion champ(ahri_data());
    const TickRate rate = champ.tick_rate();

    champ.apply_effect(flat_effect({.source = "Test", .name = "Short"},
                                   {base_mod(StatId::AttackDamage, 10)},
                                   Timed::for_span(champ.now(), rate.ticks_from_seconds(1.0))));

    champ.advance_by(rate.ticks_from_seconds(0.5));
    REQUIRE(champ.now() == Tick{30});
    REQUIRE(champ.compute(StatId::AttackDamage) == 63);

    champ.advance_by(rate.ticks_from_seconds(0.5));
    REQUIRE(champ.now() == Tick{60});
    REQUIRE(champ.compute(StatId::AttackDamage) == 53);
}

TEST_CASE("Re-casting a buff refreshes it on the champion", "[champion][effects]") {
    Champion champ(ahri_data());
    const TickRate rate = champ.tick_rate();
    const EffectKey key{.source = "Ahri", .name = "Fury"};
    const auto cast = [&] {
        champ.apply_effect(flat_effect(key, {base_mod(StatId::AttackDamage, 30)},
                                       Timed::for_span(champ.now(), rate.ticks_from_seconds(3.0))));
    };

    cast();
    champ.advance_by(rate.ticks_from_seconds(2.0));
    cast(); // re-cast with 1s left

    REQUIRE(champ.effects().size() == 1);
    REQUIRE(champ.compute(StatId::AttackDamage) == 83);

    // The refreshed buff runs 3s from the re-cast, i.e. to t=5s.
    champ.advance_by(rate.ticks_from_seconds(2.99));
    REQUIRE(champ.compute(StatId::AttackDamage) == 83);
    champ.advance_by(rate.ticks_from_seconds(0.01));
    REQUIRE(champ.compute(StatId::AttackDamage) == 53);
}

TEST_CASE("Champion effects can be dispelled by source", "[champion][effects]") {
    Champion champ(ahri_data());
    champ.apply_effect(
        flat_effect({.source = "Baron", .name = "Hand of Baron"}, {base_mod(StatId::Armor, 40)}));
    champ.apply_effect(
        flat_effect({.source = "Item", .name = "Passive"}, {base_mod(StatId::Armor, 10)}));

    REQUIRE(champ.compute(StatId::Armor) == 71);

    REQUIRE(champ.remove_effect({.source = "Baron", .name = "Hand of Baron"}) == 1);
    REQUIRE(champ.compute(StatId::Armor) == 31);
}

TEST_CASE("An effect scaling off another stat sees the final value", "[champion][effects]") {
    // The classic case: "gain armor equal to 50% of your bonus AD" must observe
    // AD after items and other buffs, not a stale intermediate value.
    Champion champ(ahri_data(), 6);
    champ.equip(Item{.name = "B.F. Sword", .modifiers = {base_mod(StatId::AttackDamage, 40)}});
    champ.apply_effect(Effect{
        .key = {.source = "Item", .name = "Steelcaps"},
        .reads = {StatId::AttackDamage},
        .writes = {StatId::Armor},
        .contribute =
            [](const EffectContext& ctx, ModifierSink& sink) {
                sink.add_base(StatId::Armor, 0.5 * ctx.stats(StatId::AttackDamage));
            },
    });
    // Applied after the conversion, but must still be seen by it.
    champ.apply_effect(
        flat_effect({.source = "Buff", .name = "AD"}, {base_mod(StatId::AttackDamage, 12)}));

    // AD: 68 + 40 + 12 = 120. Armor: 21 base + 60 = 81.
    REQUIRE(champ.compute(StatId::AttackDamage) == 120);
    REQUIRE(champ.compute(StatId::Armor) == 81);
}

TEST_CASE("Champion stats change when time passes even with no expiry", "[champion][effects]") {
    // An effect that scales with elapsed time: advancing the clock has to
    // invalidate the cached table even though nothing expired.
    Champion champ(ahri_data());
    champ.apply_effect(Effect{
        .key = {.source = "Test", .name = "Ramping"},
        .writes = {StatId::MovementSpeed},
        .contribute =
            [](const EffectContext& ctx, ModifierSink& sink) {
                sink.add_base(StatId::MovementSpeed, 10.0 * ctx.rate.seconds_at(ctx.now));
            },
    });

    REQUIRE(champ.compute(StatId::MovementSpeed) == 330);
    champ.advance_by(champ.tick_rate().ticks_from_seconds(1.0));
    REQUIRE(champ.compute(StatId::MovementSpeed) == 340);
    champ.advance_by(champ.tick_rate().ticks_from_seconds(2.0));
    REQUIRE(champ.compute(StatId::MovementSpeed) == 360);
}

TEST_CASE("A OneShot effect on a champion applies for exactly one step", "[champion][effects]") {
    Champion champ(ahri_data());
    champ.apply_effect(flat_effect({.source = "Ahri", .name = "Empowered"},
                                   {base_mod(StatId::AttackDamage, 100)}, OneShot{}));

    // Reading stats several times within the step must not consume it.
    REQUIRE(champ.compute(StatId::AttackDamage) == 153);
    REQUIRE(champ.compute(StatId::AttackDamage) == 153);

    champ.advance_by(TickSpan{1});
    REQUIRE(champ.compute(StatId::AttackDamage) == 53);
    REQUIRE(champ.effects().empty());
}

TEST_CASE("Stacking effects accumulate on a champion", "[champion][effects]") {
    Champion champ(ahri_data());
    const Effect siphon{
        .key = {.source = "Nasus", .name = "Siphoning Strike"},
        .writes = {StatId::AttackDamage},
        .policy = StackPolicy::Stack,
        .max_stacks = 5,
        .contribute = [](const EffectContext&,
                         ModifierSink& sink) { sink.add_base(StatId::AttackDamage, 3); },
    };

    for (int i = 0; i < 7; ++i) {
        champ.apply_effect(siphon);
    }

    REQUIRE(champ.effects().stacks_of(siphon.key) == 5);
    // 53 + 5 * 3 = 68
    REQUIRE(champ.compute(StatId::AttackDamage) == 68);
}

TEST_CASE("A long simulation stays exact tick by tick", "[champion][effects]") {
    // A minute of stepping with a buff re-cast once a second, each lasting half
    // a second: the number of buffed ticks is exact, not approximate. This is
    // what integral time buys — accumulating 1/60 as a double 3600 times would
    // drift and make the count depend on the tick rate.
    Champion champ(ahri_data());
    const TickRate rate = champ.tick_rate();
    const EffectKey key{.source = "Test", .name = "Pulse"};
    const TickSpan duration = rate.ticks_from_seconds(0.5); // 30 ticks

    // Cast at t = 0, 60, ..., 3540 (60 casts), simulating to t = 3599 so every
    // cast's 30 tick window closes inside the run.
    int buffed_ticks = 0;
    int casts = 0;
    for (Tick::Rep t = 0; t <= 3599; ++t) {
        champ.advance_to(Tick{t});
        if (t % 60 == 0 && t <= 3540) {
            champ.apply_effect(flat_effect(key, {base_mod(StatId::AttackDamage, 10)},
                                           Timed::for_span(champ.now(), duration)));
            ++casts;
        }
        if (champ.compute(StatId::AttackDamage) == 63) {
            ++buffed_ticks;
        }
    }

    REQUIRE(casts == 60);
    REQUIRE(buffed_ticks == 60 * 30);
    // The last window closed, so nothing is left over.
    REQUIRE(champ.effects().empty());
}
