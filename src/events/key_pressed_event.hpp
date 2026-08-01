#pragma once

#include <iosfwd>

namespace moba_sim {

struct KeyPressedEvent {
    int key;
};

void handle_event(const KeyPressedEvent& event, std::ostream& debug_out);

} // namespace moba_sim
