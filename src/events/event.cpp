#include "events/event.hpp"

#include <ostream>

#include "events/event_sequence.hpp"
#include "events/key_pressed_event.hpp"
#include "events/player_died_event.hpp"

namespace moba_sim {

void process_event(const Event& event, std::ostream& debug_out) {
    // Overload resolution picks the handle_event for the stored alternative;
    // each event type lives in its own translation unit.
    std::visit([&debug_out](const auto& e) { handle_event(e, debug_out); }, event);
}

} // namespace moba_sim
