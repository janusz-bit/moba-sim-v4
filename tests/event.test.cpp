#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <sstream>

#include "events/event.hpp"

using namespace moba_sim;

TEST_CASE("process_event handles PlayerDiedEvent", "[events]") {
    std::ostringstream out;
    process_event(Event{PlayerDiedEvent{"Anna"}}, out);
    REQUIRE(out.str() == "Gracz zginął: Anna\n");
}

TEST_CASE("process_event handles KeyPressedEvent", "[events]") {
    std::ostringstream out;
    process_event(Event{KeyPressedEvent{32}}, out);
    REQUIRE(out.str() == "Klawisz: 32\n");
}

TEST_CASE("process_event without a stream is silent and safe", "[events]") {
    auto seq = std::make_shared<EventSequence>();
    seq->events.push_back(KeyPressedEvent{32});
    // Domyślny null stream: brak wyjścia, brak wyjątków
    REQUIRE_NOTHROW(process_event(Event{seq}));
}

TEST_CASE("process_event handles an empty sequence", "[events]") {
    std::ostringstream out;
    process_event(Event{std::make_shared<EventSequence>()}, out);
    REQUIRE(out.str() == "--- Początek Sekwencji Zdarzeń ---\n"
                         "--- Koniec Sekwencji ---\n");
}

TEST_CASE("process_event handles nested sequences recursively", "[events]") {
    auto inner = std::make_shared<EventSequence>();
    inner->events.push_back(KeyPressedEvent{32});

    auto outer = std::make_shared<EventSequence>();
    outer->events.push_back(PlayerDiedEvent{"Anna"});
    outer->events.push_back(Event{inner});
    outer->events.push_back(KeyPressedEvent{13});

    std::ostringstream out;
    process_event(Event{outer}, out);

    REQUIRE(out.str() == "--- Początek Sekwencji Zdarzeń ---\n"
                         "Gracz zginął: Anna\n"
                         "--- Początek Sekwencji Zdarzeń ---\n"
                         "Klawisz: 32\n"
                         "--- Koniec Sekwencji ---\n"
                         "Klawisz: 13\n"
                         "--- Koniec Sekwencji ---\n");
}
