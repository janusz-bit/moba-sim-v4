#include <iostream>
#include <memory>

#include "champions/champion.hpp"
#include "events/event.hpp"
#include "stats/stat_breakdown.hpp"

namespace {

using namespace moba_sim;

/// https://wiki.leagueoflegends.com/en-us/Ahri — "Base statistics"
ChampionData ahri_data() {
    return ChampionData{
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
}

void show(const Champion& champ, StatId stat) {
    std::cout << format_breakdown(stat_name(stat), champ.explain(stat)) << "\n\n";
}

/// Walks a champion through a few simulation steps, showing that effects
/// expire on their own and that a stat derived from another stat sees the
/// final value in a single pass.
void stat_demo() {
    Champion ahri{ahri_data(), 6};
    const TickRate rate = ahri.tick_rate();

    ahri.equip(Item{.name = "B.F. Sword", .modifiers = {base_mod(StatId::AttackDamage, 40)}});

    // A conversion effect: 50% of total AD as armor. It declares that it reads
    // AttackDamage and writes Armor, which is how the framework knows to
    // evaluate it after everything that contributes AD — exactly once, with no
    // convergence loop.
    ahri.apply_effect(Effect{
        .key = {.source = "Steelcaps", .name = "Sturdy"},
        .reads = {StatId::AttackDamage},
        .writes = {StatId::Armor},
        .contribute =
            [](const EffectContext& ctx, ModifierSink& sink) {
                sink.add_base(StatId::Armor, 0.5 * ctx.stats(StatId::AttackDamage));
            },
    });

    // A 3 second buff. Its deadline is data the framework owns, so nobody has
    // to poll it or remember when it started.
    const EffectKey fury{.source = "Ahri", .name = "Fury"};
    ahri.apply_effect(flat_effect(fury, {base_mod(StatId::AttackDamage, 30)},
                                  Timed::for_span(ahri.now(), rate.ticks_from_seconds(3.0))));

    std::cout << "=== Ahri, level 6, B.F. Sword + Fury (3s) ===\n\n";
    show(ahri, StatId::AttackDamage);
    show(ahri, StatId::Armor);

    const auto remaining_seconds = [&] {
        const auto left = ahri.effects().remaining_on(fury, ahri.now());
        return left ? rate.seconds_from_ticks(*left) : 0.0;
    };
    std::cout << "Fury has " << remaining_seconds() << "s left\n\n";

    // Step forward 3 seconds; the buff expires on its own and says so.
    std::cout << "--- advancing 3 seconds ---\n";
    for (const EffectKey& expired : ahri.advance_by(rate.ticks_from_seconds(3.0))) {
        std::cout << "expired: " << expired.label() << "\n";
    }
    std::cout << "\n";

    std::cout << "=== after the buff expired ===\n\n";
    show(ahri, StatId::AttackDamage);
    // Armor followed AD down without anyone recomputing it by hand.
    show(ahri, StatId::Armor);
}

void event_demo() {
    std::cout << "=== events ===\n";

    // Create an event sequence (e.g. a macro in a game)
    auto seq = std::make_shared<EventSequence>();
    seq->events.push_back(KeyPressedEvent{32});
    seq->events.push_back(PlayerDiedEvent{"Anna"});

    // Process the main event containing the sequence.
    // std::cout acts as the debug output — without this argument
    // process_event would stay silent.
    const Event main_event = seq;
    process_event(main_event, std::cout);
}

} // namespace

int main() {
    stat_demo();
    event_demo();
}
