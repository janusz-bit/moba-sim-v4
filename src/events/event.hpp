#pragma once

#include <memory>
#include <ostream>
#include <string>
#include <variant>
#include <vector>

namespace moba_sim {

struct PlayerDiedEvent {
    std::string name;
};

struct KeyPressedEvent {
    int key;
};

// Aggregate event: contains a vector of other events!
struct EventSequence;

using Event = std::variant<PlayerDiedEvent, KeyPressedEvent,
                           std::shared_ptr<EventSequence> // Nesting!
                           >;

struct EventSequence {
    std::vector<Event> events;
};

namespace detail {

/// "Null" stream — discards all writes (badbit => no-op).
/// This keeps the debug output disabled by default.
inline std::ostream& null_stream() {
    static std::ostream stream{nullptr};
    return stream;
}

} // namespace detail

// Without a stream argument the function stays silent; pass e.g.
// std::cout / std::cerr to enable event printing (debug).
void process_event(const Event& event, std::ostream& debug_out = detail::null_stream());

} // namespace moba_sim
