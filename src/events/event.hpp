#pragma once

#include <memory>
#include <ostream>
#include <variant>
#include <vector>

#include "events/key_pressed_event.hpp"
#include "events/player_died_event.hpp"

namespace moba_sim {

struct EventSequence;

/// Any event the simulation can process.
///
/// A closed sum type rather than a base class with virtual handlers: the set of
/// event kinds is known at compile time, so std::visit gives exhaustive
/// dispatch that a forgotten override could not silently break.
///
/// The std::shared_ptr<EventSequence> alternative makes events recursive -- a
/// sequence is itself an event, so sequences nest arbitrarily. The indirection
/// is required because EventSequence contains a vector of Event, which cannot
/// be a complete type at this point; hence the forward declaration above and
/// the definition below.
using Event = std::variant<PlayerDiedEvent, KeyPressedEvent,
                           std::shared_ptr<EventSequence> // Nesting!
                           >;

/// An ordered group of events, processed as one.
///
/// Defined after the Event alias because it stores Events. This is the
/// recursive case of the variant above: a macro, a combo, or any compound
/// action.
struct EventSequence {
    std::vector<Event> events; ///< Sub-events, processed in order.
};

namespace detail {

/// A stream that discards everything written to it.
///
/// Constructed from a null buffer, so it holds badbit and every write is a
/// no-op. This is what makes event handling silent by default.
inline std::ostream& null_stream() {
    static std::ostream stream{nullptr};
    return stream;
}

} // namespace detail

/// Dispatches `event` to the handle_event overload matching its alternative.
///
/// Dispatch is a std::visit over free functions resolved at this call site,
/// which is why each event type's handler can live in its own translation
/// unit without a registration step.
///
/// `debug_out` defaults to a discarding stream, so processing is silent unless
/// a stream is passed; pass std::cout or std::cerr to trace events.
void process_event(const Event& event, std::ostream& debug_out = detail::null_stream());

} // namespace moba_sim
