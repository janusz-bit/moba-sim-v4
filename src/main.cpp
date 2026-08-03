#include <iostream>
#include <memory>

#include "events/event.hpp"

int main() {
    // Create an event sequence (e.g. a macro in a game)
    auto seq = std::make_shared<moba_sim::EventSequence>();
    seq->events.push_back(moba_sim::KeyPressedEvent{32});
    seq->events.push_back(moba_sim::PlayerDiedEvent{"Anna"});

    // Process the main event containing the sequence.
    // std::cout acts as the debug output — without this argument
    // process_event would stay silent.
    moba_sim::Event main_event = seq;
    moba_sim::process_event(main_event, std::cout);
}
