#include "events/event_sequence.hpp"

#include <ostream>

#include "events/event.hpp"

namespace moba_sim {

void handle_event(const std::unique_ptr<EventSequence>& sequence, std::ostream& debug_out) {
    debug_out << "--- Begin Event Sequence ---\n";
    for (const auto& sub_event : sequence->events) {
        // Recursion: sub-events go through the main dispatcher again
        process_event(sub_event, debug_out);
    }
    debug_out << "--- End Event Sequence ---\n";
}

} // namespace moba_sim
