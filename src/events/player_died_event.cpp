#include "events/player_died_event.hpp"

#include <ostream>

namespace moba_sim {

void handle_event(const PlayerDiedEvent& event, std::ostream& debug_out) {
    debug_out << "Player died: " << event.name << "\n";
}

} // namespace moba_sim
