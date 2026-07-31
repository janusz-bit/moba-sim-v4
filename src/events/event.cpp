#include "events/event.hpp"

#include <ostream>
#include <type_traits>

namespace moba_sim {

void process_event(const Event& event, std::ostream& debug_out) {
    // C++23: the lambda uses `self` (deducing this) to recursively
    // process sub-events
    std::visit(
        [&debug_out](this auto self, const auto& e) -> void {
            using T = std::decay_t<decltype(e)>;

            if constexpr (std::is_same_v<T, PlayerDiedEvent>) {
                debug_out << "Player died: " << e.name << "\n";
            } else if constexpr (std::is_same_v<T, KeyPressedEvent>) {
                debug_out << "Key pressed: " << e.key << "\n";
            } else if constexpr (std::is_same_v<T, std::shared_ptr<EventSequence>>) {
                debug_out << "--- Begin Event Sequence ---\n";
                for (const auto& sub_event : e->events) {
                    // RECURSION! We pass 'self' (i.e. this very lambda)
                    // to the next visit
                    std::visit(self, sub_event);
                }
                debug_out << "--- End Event Sequence ---\n";
            }
        },
        event);
}

} // namespace moba_sim
