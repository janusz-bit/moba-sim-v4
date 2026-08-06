#pragma once

#include <iosfwd>

namespace moba_sim {

/// A key press, identified by its raw key code.
///
/// Deliberately not an SDL type: the event layer lives in the headless core,
/// so input is reduced to a plain integer at the boundary.
struct KeyPressedEvent {
    int key = 0; ///< Raw key code, as reported by the input source.
};

/// Handles a key press by describing it on `debug_out`.
///
/// Found by overload resolution from process_event's std::visit; see
/// events/event.hpp for how dispatch works.
void handle_event(const KeyPressedEvent& event, std::ostream& debug_out);

} // namespace moba_sim
