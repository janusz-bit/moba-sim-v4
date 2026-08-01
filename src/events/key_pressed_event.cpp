#include "events/key_pressed_event.hpp"

#include <ostream>

namespace moba_sim {

void handle_event(const KeyPressedEvent& event, std::ostream& debug_out) {
    debug_out << "Key pressed: " << event.key << "\n";
}

} // namespace moba_sim
