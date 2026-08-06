#pragma once

#include <iosfwd>
#include <string>

namespace moba_sim {

/// A player's death, naming who died.
struct PlayerDiedEvent {
    std::string name; ///< Name of the player who died.
};

/// Handles a death by describing it on `debug_out`.
///
/// Found by overload resolution from process_event's std::visit; see
/// events/event.hpp for how dispatch works.
void handle_event(const PlayerDiedEvent& event, std::ostream& debug_out);

} // namespace moba_sim
