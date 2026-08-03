#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <sstream>

#include "events/event.hpp"

using namespace moba_sim;

TEST_CASE("process_event handles PlayerDiedEvent", "[events]") {
    std::ostringstream out;
    process_event(Event{PlayerDiedEvent{"Anna"}}, out);
    REQUIRE(out.str() == "Player died: Anna\n");
}

TEST_CASE("process_event handles KeyPressedEvent", "[events]") {
    std::ostringstream out;
    process_event(Event{KeyPressedEvent{32}}, out);
    REQUIRE(out.str() == "Key pressed: 32\n");
}

TEST_CASE("process_event without a stream is silent and safe", "[events]") {
    auto seq = std::make_shared<EventSequence>();
    seq->events.push_back(KeyPressedEvent{32});
    // Default null stream: no output, no exceptions
    REQUIRE_NOTHROW(process_event(Event{seq}));
}

TEST_CASE("process_event handles an empty sequence", "[events]") {
    std::ostringstream out;
    process_event(Event{std::make_shared<EventSequence>()}, out);
    REQUIRE(out.str() == "--- Begin Event Sequence ---\n"
                         "--- End Event Sequence ---\n");
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

    REQUIRE(out.str() == "--- Begin Event Sequence ---\n"
                         "Player died: Anna\n"
                         "--- Begin Event Sequence ---\n"
                         "Key pressed: 32\n"
                         "--- End Event Sequence ---\n"
                         "Key pressed: 13\n"
                         "--- End Event Sequence ---\n");
}
