#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "effects/effect_set.hpp"
#include "stats/stat_table.hpp"

using namespace moba_sim;

namespace {

/// A stat table seeded with one Base value, standing in for a champion's base
/// stats so effect behaviour can be tested without a Champion.
StatTable seeded(StatId stat, double value) {
    StatTable table;
    table[stat].add_base(value, "base");
    return table;
}

/// Resolves `set` against a freshly seeded table and returns the stat value.
double resolve(const EffectSet& set, StatId stat, double base, Tick now) {
    StatTable table = seeded(stat, base);
    set.contribute_all(table, now);
    return table.compute(stat);
}

Effect flat_ad(std::string name, double amount, Lifetime lifetime = Permanent{}) {
    return flat_effect({.source = "Test", .name = std::move(name)},
                       {base_mod(StatId::AttackDamage, amount)}, std::move(lifetime));
}

} // namespace

// --- lifetime as data ------------------------------------------------------

TEST_CASE("Permanent effects survive advancing time", "[effects]") {
    EffectSet set;
    set.apply(flat_ad("Forever", 50), kSimulationStart);

    for (int i = 1; i <= 1000; ++i) {
        REQUIRE(set.advance(Tick{i}).empty());
    }
    REQUIRE(set.size() == 1);
    REQUIRE(resolve(set, StatId::AttackDamage, 100, Tick{1000}) == 150);
}

TEST_CASE("Timed effects expire at their deadline, not before", "[effects]") {
    EffectSet set;
    const TickSpan duration{60};
    set.apply(flat_ad("Rage", 30, Timed::for_span(kSimulationStart, duration)), kSimulationStart);

    // Alive for ticks 0..59; the value is unchanged the whole time.
    for (Tick::Rep t = 0; t < 60; ++t) {
        REQUIRE(set.advance(Tick{t}).empty());
        REQUIRE(resolve(set, StatId::AttackDamage, 100, Tick{t}) == 130);
    }

    // Gone exactly on tick 60.
    const auto expired = set.advance(Tick{60});
    REQUIRE(expired.size() == 1);
    REQUIRE(expired[0].name == "Rage");
    REQUIRE(set.empty());
    REQUIRE(resolve(set, StatId::AttackDamage, 100, Tick{60}) == 100);
}

TEST_CASE("Expiry is exact regardless of how time is stepped", "[effects]") {
    // Whether the caller advances tick by tick or jumps, the effect covers the
    // same span. Integral time is what makes this hold.
    const TickSpan duration{60};

    EffectSet stepped;
    stepped.apply(flat_ad("A", 30, Timed::for_span(kSimulationStart, duration)), kSimulationStart);
    for (Tick::Rep t = 1; t <= 60; ++t) {
        stepped.advance(Tick{t});
    }

    EffectSet jumped;
    jumped.apply(flat_ad("A", 30, Timed::for_span(kSimulationStart, duration)), kSimulationStart);
    jumped.advance(Tick{60});

    REQUIRE(stepped.empty());
    REQUIRE(jumped.empty());
}

TEST_CASE("OneShot effects contribute once and are retired", "[effects]") {
    EffectSet set;
    set.apply(flat_ad("Empowered", 100, OneShot{}), kSimulationStart);

    // Contributes on the step it was applied on...
    REQUIRE(set.advance(kSimulationStart).empty());
    REQUIRE(resolve(set, StatId::AttackDamage, 100, kSimulationStart) == 200);

    // ...and is gone on the next one.
    const auto expired = set.advance(Tick{1});
    REQUIRE(expired.size() == 1);
    REQUIRE(set.empty());
    REQUIRE(resolve(set, StatId::AttackDamage, 100, Tick{1}) == 100);
}

TEST_CASE("Evaluating a OneShot many times does not consume it", "[effects]") {
    // Because contribute is pure and removal happens only in advance(), reading
    // stats repeatedly within one step cannot change what expires. The old
    // "alive flag returned from the same call that computes stats" design made
    // this depend on how many times the solver iterated.
    EffectSet set;
    set.apply(flat_ad("Empowered", 100, OneShot{}), kSimulationStart);

    for (int i = 0; i < 20; ++i) {
        REQUIRE(resolve(set, StatId::AttackDamage, 100, kSimulationStart) == 200);
    }
    REQUIRE(set.size() == 1);
}

TEST_CASE("Until effects live while their predicate holds", "[effects]") {
    EffectSet set;
    set.apply(flat_ad("Below half", 40, Until{[](Tick now) { return now < Tick{5}; }}),
              kSimulationStart);

    REQUIRE(set.advance(Tick{4}).empty());
    REQUIRE(set.size() == 1);
    REQUIRE(set.advance(Tick{5}).size() == 1);
    REQUIRE(set.empty());
}

TEST_CASE("An Until without a predicate expires instead of living forever", "[effects]") {
    EffectSet set;
    set.apply(flat_ad("Broken", 40, Until{}), kSimulationStart);

    REQUIRE(set.advance(kSimulationStart).size() == 1);
    REQUIRE(set.empty());
}

TEST_CASE("Remaining duration is visible to the framework", "[effects]") {
    // The question a buff bar asks. Unanswerable when the deadline lives in a
    // lambda capture.
    EffectSet set;
    const EffectKey key{.source = "Test", .name = "Rage"};
    set.apply(flat_effect(key, {base_mod(StatId::AttackDamage, 30)},
                          Timed::for_span(kSimulationStart, TickSpan{60})),
              kSimulationStart);

    REQUIRE(set.remaining_on(key, kSimulationStart) == TickSpan{60});
    REQUIRE(set.remaining_on(key, Tick{45}) == TickSpan{15});
    REQUIRE(set.remaining_on(key, Tick{60}) == TickSpan{0});
    REQUIRE_FALSE(set.remaining_on({.source = "Test", .name = "Absent"}, kSimulationStart));
}

TEST_CASE("Permanent effects report no remaining duration", "[effects]") {
    EffectSet set;
    const EffectKey key{.source = "Item", .name = "Passive"};
    set.apply(flat_effect(key, {base_mod(StatId::AttackDamage, 30)}), kSimulationStart);

    REQUIRE_FALSE(set.remaining_on(key, kSimulationStart).has_value());
}

// --- identity and stacking -------------------------------------------------

TEST_CASE("The same key re-applied refreshes instead of stacking", "[effects]") {
    // Identity is semantic, so re-casting the same buff is recognised as the
    // same buff. A per-application counter id would have created a second
    // instance here and doubled the bonus.
    EffectSet set;
    const auto make = [] {
        return flat_ad("Rage", 30, Timed::for_span(kSimulationStart, TickSpan{60}));
    };

    set.apply(make(), kSimulationStart);
    set.apply(make(), kSimulationStart);

    REQUIRE(set.size() == 1);
    REQUIRE(resolve(set, StatId::AttackDamage, 100, kSimulationStart) == 130);
}

TEST_CASE("Refresh resets the duration from the new application time", "[effects]") {
    EffectSet set;
    const EffectKey key{.source = "Test", .name = "Rage"};
    const auto cast_at = [&](Tick now) {
        set.apply(flat_effect(key, {base_mod(StatId::AttackDamage, 30)},
                              Timed::for_span(now, TickSpan{60})),
                  now);
    };

    cast_at(kSimulationStart);
    cast_at(Tick{40});

    // Re-cast at 40 pushes expiry to 100, not 60. One instance, not two.
    REQUIRE(set.size() == 1);
    REQUIRE(set.remaining_on(key, Tick{40}) == TickSpan{60});
    set.advance(Tick{99});
    REQUIRE(set.size() == 1);
    set.advance(Tick{100});
    REQUIRE(set.empty());
}

TEST_CASE("Stack policy accumulates intensity up to max_stacks", "[effects]") {
    EffectSet set;
    const Effect siphon{
        .key = {.source = "Nasus", .name = "Siphoning Strike"},
        .writes = {StatId::AttackDamage},
        .policy = StackPolicy::Stack,
        .max_stacks = 3,
        .contribute = [](const EffectContext&,
                         ModifierSink& sink) { sink.add_base(StatId::AttackDamage, 10); },
    };

    set.apply(siphon, kSimulationStart);
    REQUIRE(set.stacks_of(siphon.key) == 1);
    REQUIRE(resolve(set, StatId::AttackDamage, 100, kSimulationStart) == 110);

    set.apply(siphon, kSimulationStart);
    REQUIRE(set.stacks_of(siphon.key) == 2);
    REQUIRE(resolve(set, StatId::AttackDamage, 100, kSimulationStart) == 120);

    set.apply(siphon, kSimulationStart);
    set.apply(siphon, kSimulationStart);
    // Capped at 3 stacks, and still a single instance.
    REQUIRE(set.stacks_of(siphon.key) == 3);
    REQUIRE(set.size() == 1);
    REQUIRE(resolve(set, StatId::AttackDamage, 100, kSimulationStart) == 130);
}

TEST_CASE("Stacked More modifiers compound rather than sum", "[effects]") {
    EffectSet set;
    const Effect stacking{
        .key = {.source = "Test", .name = "Compounding"},
        .writes = {StatId::AttackDamage},
        .policy = StackPolicy::Stack,
        .max_stacks = 3,
        .contribute = [](const EffectContext&,
                         ModifierSink& sink) { sink.add_more(StatId::AttackDamage, 0.1); },
    };

    set.apply(stacking, kSimulationStart);
    set.apply(stacking, kSimulationStart);
    set.apply(stacking, kSimulationStart);

    // Three stacks of +10% More: 100 * 1.1^3 = 133.1
    REQUIRE_THAT(resolve(set, StatId::AttackDamage, 100, kSimulationStart),
                 Catch::Matchers::WithinAbs(133.1, 1e-9));
}

TEST_CASE("ExtendDuration adds to what is left instead of resetting", "[effects]") {
    EffectSet set;
    const EffectKey key{.source = "Test", .name = "Extending"};
    const auto apply_at = [&](Tick now) {
        Effect effect = flat_effect(key, {base_mod(StatId::AttackDamage, 30)},
                                    Timed::for_span(now, TickSpan{60}));
        effect.policy = StackPolicy::ExtendDuration;
        set.apply(std::move(effect), now);
    };

    apply_at(kSimulationStart);
    apply_at(Tick{20}); // 40 ticks left + 60 new = 100 remaining

    REQUIRE(set.size() == 1);
    REQUIRE(set.remaining_on(key, Tick{20}) == TickSpan{100});
}

TEST_CASE("IgnoreIfPresent drops re-application while active", "[effects]") {
    EffectSet set;
    const EffectKey key{.source = "Test", .name = "Once"};

    Effect effect = flat_effect(key, {base_mod(StatId::AttackDamage, 30)},
                                Timed::for_span(kSimulationStart, TickSpan{60}));
    effect.policy = StackPolicy::IgnoreIfPresent;

    const auto first = set.apply(effect, kSimulationStart);
    const auto second = set.apply(effect, Tick{30});

    REQUIRE(first.has_value());
    REQUIRE_FALSE(second.has_value());
    // The original deadline is untouched: 30 ticks left at t=30.
    REQUIRE(set.remaining_on(key, Tick{30}) == TickSpan{30});
}

TEST_CASE("ReplaceIfStronger keeps the strongest instance", "[effects]") {
    EffectSet set;
    const EffectKey key{.source = "Aura", .name = "Might"};
    const auto aura = [&](double amount) {
        Effect effect = flat_effect(key, {base_mod(StatId::AttackDamage, amount)});
        effect.policy = StackPolicy::ReplaceIfStronger;
        effect.magnitude = amount;
        return effect;
    };

    set.apply(aura(20), kSimulationStart);
    REQUIRE(resolve(set, StatId::AttackDamage, 100, kSimulationStart) == 120);

    // A weaker source of the same aura does not overwrite the stronger one.
    REQUIRE_FALSE(set.apply(aura(10), kSimulationStart).has_value());
    REQUIRE(resolve(set, StatId::AttackDamage, 100, kSimulationStart) == 120);

    // A stronger one does.
    REQUIRE(set.apply(aura(35), kSimulationStart).has_value());
    REQUIRE(resolve(set, StatId::AttackDamage, 100, kSimulationStart) == 135);
    REQUIRE(set.size() == 1);
}

TEST_CASE("ReplaceIfStronger prefers a later expiry at equal magnitude", "[effects]") {
    EffectSet set;
    const EffectKey key{.source = "Aura", .name = "Might"};
    const auto aura = [&](TickSpan duration) {
        Effect effect = flat_effect(key, {base_mod(StatId::AttackDamage, 20)},
                                    Timed::for_span(kSimulationStart, duration));
        effect.policy = StackPolicy::ReplaceIfStronger;
        effect.magnitude = 20;
        return effect;
    };

    set.apply(aura(TickSpan{60}), kSimulationStart);
    REQUIRE_FALSE(set.apply(aura(TickSpan{30}), kSimulationStart).has_value());
    REQUIRE(set.remaining_on(key, kSimulationStart) == TickSpan{60});
    REQUIRE(set.apply(aura(TickSpan{120}), kSimulationStart).has_value());
    REQUIRE(set.remaining_on(key, kSimulationStart) == TickSpan{120});
}

TEST_CASE("Independent effects with one key coexist and expire separately", "[effects]") {
    EffectSet set;
    const EffectKey key{.source = "Teemo", .name = "Toxic Shot"};
    const auto dot = [&](Tick now, TickSpan duration) {
        Effect effect =
            flat_effect(key, {base_mod(StatId::AttackDamage, 10)}, Timed::for_span(now, duration));
        effect.policy = StackPolicy::Independent;
        return effect;
    };

    set.apply(dot(kSimulationStart, TickSpan{30}), kSimulationStart);
    set.apply(dot(kSimulationStart, TickSpan{60}), kSimulationStart);

    REQUIRE(set.size() == 2);
    REQUIRE(resolve(set, StatId::AttackDamage, 100, kSimulationStart) == 120);

    set.advance(Tick{30});
    REQUIRE(set.size() == 1);
    REQUIRE(resolve(set, StatId::AttackDamage, 100, Tick{30}) == 110);

    set.advance(Tick{60});
    REQUIRE(set.empty());
}

// --- removal ---------------------------------------------------------------

TEST_CASE("Effects can be removed by key", "[effects]") {
    EffectSet set;
    const EffectKey rage{.source = "Test", .name = "Rage"};
    set.apply(flat_effect(rage, {base_mod(StatId::AttackDamage, 30)}), kSimulationStart);
    set.apply(flat_ad("Other", 5), kSimulationStart);

    REQUIRE(set.remove(rage) == 1);
    REQUIRE(set.size() == 1);
    REQUIRE(set.remove(rage) == 0);
    REQUIRE(resolve(set, StatId::AttackDamage, 100, kSimulationStart) == 105);
}

TEST_CASE("A specific instance can be removed by handle", "[effects]") {
    EffectSet set;
    const EffectKey key{.source = "Test", .name = "Dot"};
    const auto independent = [&] {
        Effect effect = flat_effect(key, {base_mod(StatId::AttackDamage, 10)});
        effect.policy = StackPolicy::Independent;
        return effect;
    };

    const auto first = set.apply(independent(), kSimulationStart);
    set.apply(independent(), kSimulationStart);
    REQUIRE(set.size() == 2);

    REQUIRE(set.remove(*first));
    REQUIRE(set.size() == 1);
    REQUIRE_FALSE(set.remove(*first));
}

TEST_CASE("remove_if implements dispel over keys", "[effects]") {
    EffectSet set;
    set.apply(
        flat_effect({.source = "Baron", .name = "Buff"}, {base_mod(StatId::AttackDamage, 10)}),
        kSimulationStart);
    set.apply(
        flat_effect({.source = "Baron", .name = "Other"}, {base_mod(StatId::AttackDamage, 20)}),
        kSimulationStart);
    set.apply(
        flat_effect({.source = "Item", .name = "Passive"}, {base_mod(StatId::AttackDamage, 5)}),
        kSimulationStart);

    const std::size_t removed =
        set.remove_if([](const EffectKey& key) { return key.source == "Baron"; });

    REQUIRE(removed == 2);
    REQUIRE(set.size() == 1);
    REQUIRE(resolve(set, StatId::AttackDamage, 100, kSimulationStart) == 105);
}

TEST_CASE("clear removes everything", "[effects]") {
    EffectSet set;
    set.apply(flat_ad("A", 10), kSimulationStart);
    set.apply(flat_ad("B", 20), kSimulationStart);

    set.clear();
    REQUIRE(set.empty());
    REQUIRE(resolve(set, StatId::AttackDamage, 100, kSimulationStart) == 100);
}

// --- dependency ordering ---------------------------------------------------

TEST_CASE("An effect reading a stat sees contributions from earlier effects", "[effects]") {
    // The case that a fixed-point solver was needed for: a conversion effect
    // that must observe the final value of what it reads. Declared reads let
    // this be a single exact pass instead of iterating to a tolerance.
    EffectSet set;

    // Applied second, but must be evaluated first: it writes what the other reads.
    Effect conversion{
        .key = {.source = "Item", .name = "AD to Armor"},
        .reads = {StatId::AttackDamage},
        .writes = {StatId::Armor},
        .contribute =
            [](const EffectContext& ctx, ModifierSink& sink) {
                sink.add_base(StatId::Armor, 0.5 * ctx.stats(StatId::AttackDamage));
            },
    };
    Effect ad_buff =
        flat_effect({.source = "Buff", .name = "AD"}, {base_mod(StatId::AttackDamage, 100)});

    set.apply(conversion, kSimulationStart);
    set.apply(ad_buff, kSimulationStart);

    StatTable table;
    table[StatId::AttackDamage].add_base(100, "base");
    table[StatId::Armor].add_base(20, "base");
    set.contribute_all(table, kSimulationStart);

    // AD ends at 200, so Armor gets 100 on top of its base 20 — the conversion
    // saw the *final* AD even though it was applied first.
    REQUIRE(table.compute(StatId::AttackDamage) == 200);
    REQUIRE(table.compute(StatId::Armor) == 120);
}

TEST_CASE("Evaluation order is independent of application order", "[effects]") {
    const auto build = [](bool conversion_first) {
        EffectSet set;
        Effect conversion{
            .key = {.source = "Item", .name = "AD to Armor"},
            .reads = {StatId::AttackDamage},
            .writes = {StatId::Armor},
            .contribute =
                [](const EffectContext& ctx, ModifierSink& sink) {
                    sink.add_base(StatId::Armor, 0.5 * ctx.stats(StatId::AttackDamage));
                },
        };
        Effect ad_buff =
            flat_effect({.source = "Buff", .name = "AD"}, {base_mod(StatId::AttackDamage, 100)});
        if (conversion_first) {
            set.apply(conversion, kSimulationStart);
            set.apply(ad_buff, kSimulationStart);
        } else {
            set.apply(ad_buff, kSimulationStart);
            set.apply(conversion, kSimulationStart);
        }

        StatTable table;
        table[StatId::AttackDamage].add_base(100, "base");
        table[StatId::Armor].add_base(20, "base");
        set.contribute_all(table, kSimulationStart);
        return table.compute(StatId::Armor);
    };

    REQUIRE(build(true) == build(false));
    REQUIRE(build(true) == 120);
}

TEST_CASE("A chain of conversions resolves in one pass", "[effects]") {
    // AD -> Armor -> MagicResist. A three-link chain would take several
    // iterations to settle under a fixed-point scheme; here it is exact.
    EffectSet set;
    set.apply(
        Effect{
            .key = {.source = "C", .name = "Armor to MR"},
            .reads = {StatId::Armor},
            .writes = {StatId::MagicResist},
            .contribute =
                [](const EffectContext& ctx, ModifierSink& sink) {
                    sink.add_base(StatId::MagicResist, 0.5 * ctx.stats(StatId::Armor));
                },
        },
        kSimulationStart);
    set.apply(
        Effect{
            .key = {.source = "B", .name = "AD to Armor"},
            .reads = {StatId::AttackDamage},
            .writes = {StatId::Armor},
            .contribute =
                [](const EffectContext& ctx, ModifierSink& sink) {
                    sink.add_base(StatId::Armor, 0.5 * ctx.stats(StatId::AttackDamage));
                },
        },
        kSimulationStart);
    set.apply(flat_effect({.source = "A", .name = "AD"}, {base_mod(StatId::AttackDamage, 100)}),
              kSimulationStart);

    StatTable table;
    table[StatId::AttackDamage].add_base(100, "base");
    set.contribute_all(table, kSimulationStart);

    // AD 200 -> Armor 100 -> MR 50
    REQUIRE(table.compute(StatId::AttackDamage) == 200);
    REQUIRE(table.compute(StatId::Armor) == 100);
    REQUIRE(table.compute(StatId::MagicResist) == 50);
}

TEST_CASE("An effect may read the stat it writes: layering, not a cycle", "[effects]") {
    // "Gain 10% of your total AD as bonus AD" reads the value accumulated so
    // far. This is well-defined and must not be rejected as circular.
    EffectSet set;
    set.apply(flat_effect({.source = "Base", .name = "AD"}, {base_mod(StatId::AttackDamage, 100)}),
              kSimulationStart);
    REQUIRE_NOTHROW(set.apply(
        Effect{
            .key = {.source = "Item", .name = "Amplifier"},
            .reads = {StatId::AttackDamage},
            .writes = {StatId::AttackDamage},
            .contribute =
                [](const EffectContext& ctx, ModifierSink& sink) {
                    sink.add_base(StatId::AttackDamage, 0.1 * ctx.stats(StatId::AttackDamage));
                },
        },
        kSimulationStart));

    StatTable table;
    table[StatId::AttackDamage].add_base(100, "base");
    set.contribute_all(table, kSimulationStart);

    // 100 base + 100 buff = 200, then +10% of 200 = 220. Exactly one layer of
    // amplification — not a geometric series iterated to convergence.
    REQUIRE(table.compute(StatId::AttackDamage) == 220);
}

TEST_CASE("Two amplifiers of the same stat are peers, not a cycle", "[effects]") {
    EffectSet set;
    const auto amplifier = [](std::string name, double factor) {
        return Effect{
            .key = {.source = "Item", .name = std::move(name)},
            .reads = {StatId::AttackDamage},
            .writes = {StatId::AttackDamage},
            .contribute =
                [factor](const EffectContext& ctx, ModifierSink& sink) {
                    sink.add_base(StatId::AttackDamage, factor * ctx.stats(StatId::AttackDamage));
                },
        };
    };

    REQUIRE_NOTHROW(set.apply(amplifier("First", 0.1), kSimulationStart));
    REQUIRE_NOTHROW(set.apply(amplifier("Second", 0.1), kSimulationStart));
    REQUIRE(set.size() == 2);
}

TEST_CASE("A genuine conversion cycle is rejected when applied", "[effects]") {
    // AD -> Armor and Armor -> AD have no defined answer. Catching it here, as
    // a modelling error with the offending effects named, beats discovering it
    // as a failure to converge in the middle of a simulation.
    EffectSet set;
    set.apply(
        Effect{
            .key = {.source = "A", .name = "AD to Armor"},
            .reads = {StatId::AttackDamage},
            .writes = {StatId::Armor},
            .contribute = [](const EffectContext&, ModifierSink&) {},
        },
        kSimulationStart);

    REQUIRE_THROWS_AS(set.apply(
                          Effect{
                              .key = {.source = "B", .name = "Armor to AD"},
                              .reads = {StatId::Armor},
                              .writes = {StatId::AttackDamage},
                              .contribute = [](const EffectContext&, ModifierSink&) {},
                          },
                          kSimulationStart),
                      EffectCycleError);

    // The rejected effect left no trace.
    REQUIRE(set.size() == 1);
}

TEST_CASE("A rejected cycle names the effects involved", "[effects]") {
    EffectSet set;
    set.apply(
        Effect{
            .key = {.source = "Sheen", .name = "Spellblade"},
            .reads = {StatId::AttackDamage},
            .writes = {StatId::Armor},
            .contribute = [](const EffectContext&, ModifierSink&) {},
        },
        kSimulationStart);

    try {
        set.apply(
            Effect{
                .key = {.source = "Thornmail", .name = "Reflect"},
                .reads = {StatId::Armor},
                .writes = {StatId::AttackDamage},
                .contribute = [](const EffectContext&, ModifierSink&) {},
            },
            kSimulationStart);
        FAIL("expected EffectCycleError");
    } catch (const EffectCycleError& error) {
        const std::string what = error.what();
        CHECK(what.find("Sheen (Spellblade)") != std::string::npos);
        CHECK(what.find("Thornmail (Reflect)") != std::string::npos);
    }
}

TEST_CASE("Refreshing an effect that would self-cycle is allowed", "[effects]") {
    // Re-applying an effect replaces the incumbent, so it must not be checked
    // for cycles against the very instance it is about to displace.
    EffectSet set;
    const auto conversion = [] {
        return Effect{
            .key = {.source = "Item", .name = "AD to Armor"},
            .reads = {StatId::AttackDamage},
            .writes = {StatId::Armor},
            .contribute = [](const EffectContext&, ModifierSink&) {},
        };
    };

    set.apply(conversion(), kSimulationStart);
    REQUIRE_NOTHROW(set.apply(conversion(), Tick{10}));
    REQUIRE(set.size() == 1);
}

// --- declaration enforcement ----------------------------------------------

TEST_CASE("Reading an undeclared stat throws", "[effects]") {
    // Ordering is only sound if the declarations are complete, so an
    // undeclared read is an error rather than a silently stale number.
    EffectSet set;
    set.apply(
        Effect{
            .key = {.source = "Buggy", .name = "Reader"},
            .reads = {StatId::AttackDamage},
            .writes = {StatId::Armor},
            .contribute =
                [](const EffectContext& ctx, ModifierSink& sink) {
                    sink.add_base(StatId::Armor, ctx.stats(StatId::Health)); // not declared
                },
        },
        kSimulationStart);

    StatTable table;
    REQUIRE_THROWS_AS(set.contribute_all(table, kSimulationStart), UndeclaredStatAccess);
}

TEST_CASE("Writing an undeclared stat throws", "[effects]") {
    EffectSet set;
    set.apply(
        Effect{
            .key = {.source = "Buggy", .name = "Writer"},
            .writes = {StatId::Armor},
            .contribute =
                [](const EffectContext&, ModifierSink& sink) {
                    sink.add_base(StatId::Health, 100); // not declared
                },
        },
        kSimulationStart);

    StatTable table;
    REQUIRE_THROWS_AS(set.contribute_all(table, kSimulationStart), UndeclaredStatAccess);
}

TEST_CASE("The undeclared access error names the effect and the stat", "[effects]") {
    EffectSet set;
    set.apply(
        Effect{
            .key = {.source = "Buggy", .name = "Reader"},
            .writes = {StatId::Armor},
            .contribute =
                [](const EffectContext& ctx, ModifierSink& sink) {
                    sink.add_base(StatId::Armor, ctx.stats(StatId::MovementSpeed));
                },
        },
        kSimulationStart);

    StatTable table;
    try {
        set.contribute_all(table, kSimulationStart);
        FAIL("expected UndeclaredStatAccess");
    } catch (const UndeclaredStatAccess& error) {
        CHECK(error.stat() == StatId::MovementSpeed);
        const std::string what = error.what();
        CHECK(what.find("Buggy (Reader)") != std::string::npos);
        CHECK(what.find("MovementSpeed") != std::string::npos);
    }
}

// --- purity ----------------------------------------------------------------

TEST_CASE("contribute_all is pure: repeating it changes nothing", "[effects]") {
    EffectSet set;
    set.apply(flat_ad("A", 30), kSimulationStart);
    set.apply(
        Effect{
            .key = {.source = "B", .name = "Conversion"},
            .reads = {StatId::AttackDamage},
            .writes = {StatId::Armor},
            .contribute =
                [](const EffectContext& ctx, ModifierSink& sink) {
                    sink.add_base(StatId::Armor, 0.5 * ctx.stats(StatId::AttackDamage));
                },
        },
        kSimulationStart);

    double previous = 0.0;
    for (int i = 0; i < 5; ++i) {
        StatTable table;
        table[StatId::AttackDamage].add_base(100, "base");
        set.contribute_all(table, kSimulationStart);
        const double armor = table.compute(StatId::Armor);
        if (i > 0) {
            REQUIRE(armor == previous);
        }
        previous = armor;
        REQUIRE(set.size() == 2);
    }
    REQUIRE(previous == 65.0);
}

TEST_CASE("written_stats reports the union of effect writes", "[effects]") {
    EffectSet set;
    set.apply(flat_ad("A", 10), kSimulationStart);
    set.apply(flat_effect({.source = "B", .name = "HP"}, {base_mod(StatId::Health, 100)}),
              kSimulationStart);

    const StatMask written = set.written_stats();
    REQUIRE(written.contains(StatId::AttackDamage));
    REQUIRE(written.contains(StatId::Health));
    REQUIRE_FALSE(written.contains(StatId::Armor));
    REQUIRE(written.size() == 2);
}

TEST_CASE("An effect with no contribute callable is skipped", "[effects]") {
    EffectSet set;
    set.apply(Effect{.key = {.source = "Empty", .name = "Marker"}}, kSimulationStart);

    StatTable table;
    table[StatId::AttackDamage].add_base(100, "base");
    REQUIRE_NOTHROW(set.contribute_all(table, kSimulationStart));
    REQUIRE(table.compute(StatId::AttackDamage) == 100);
}

TEST_CASE("Effects can reason in seconds through the tick rate", "[effects]") {
    EffectSet set;
    set.apply(
        Effect{
            .key = {.source = "Test", .name = "Ramping"},
            .writes = {StatId::MovementSpeed},
            .contribute =
                [](const EffectContext& ctx, ModifierSink& sink) {
                    // +10 MS per elapsed second.
                    const double seconds = ctx.rate.seconds_at(ctx.now);
                    sink.add_base(StatId::MovementSpeed, 10.0 * seconds);
                },
        },
        kSimulationStart);

    StatTable table;
    table[StatId::MovementSpeed].add_base(300, "base");
    set.contribute_all(table, Tick{120}, TickRate{60}); // 2 seconds in
    REQUIRE(table.compute(StatId::MovementSpeed) == 320);
}
