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

// Zdarzenie zbiorcze: zawiera wektor innych zdarzeń!
struct EventSequence;

using Event = std::variant<PlayerDiedEvent, KeyPressedEvent,
                           std::shared_ptr<EventSequence> // Zagnieżdżenie!
                           >;

struct EventSequence {
    std::vector<Event> events;
};

namespace detail {

/// Strumień "null" — ignoruje wszystkie zapisy (badbit => no-op).
/// Dzięki temu debug output jest domyślnie wyłączony.
inline std::ostream& null_stream() {
    static std::ostream stream{nullptr};
    return stream;
}

} // namespace detail

// Bez podania strumienia funkcja jest cicha; podaj np. std::cout / std::cerr,
// aby włączyć wypisywanie zdarzeń (debug).
void process_event(const Event& event, std::ostream& debug_out = detail::null_stream());

} // namespace moba_sim
