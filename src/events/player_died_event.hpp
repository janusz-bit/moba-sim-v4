#pragma once

#include <iosfwd>
#include <string>

namespace moba_sim {

struct PlayerDiedEvent {
    std::string name;
};

void handle_event(const PlayerDiedEvent& event, std::ostream& debug_out);

} // namespace moba_sim
