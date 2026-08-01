#pragma once

#include <iosfwd>
#include <memory>

namespace moba_sim {

// Defined in event.hpp (it needs the Event alias, which needs this type).
struct EventSequence;

void handle_event(const std::unique_ptr<EventSequence>& sequence, std::ostream& debug_out);

} // namespace moba_sim
