#pragma once

#include <iosfwd>
#include <memory>

namespace moba_sim {

/// Defined in events/event.hpp, which needs the Event alias that in turn needs
/// this type -- hence the forward declaration here.
struct EventSequence;

/// Handles a sequence by processing every sub-event in order.
///
/// Recurses through process_event rather than calling handlers directly, so
/// nested sequences work at any depth and every sub-event goes through the same
/// dispatch path.
void handle_event(const std::shared_ptr<EventSequence>& sequence, std::ostream& debug_out);

} // namespace moba_sim
