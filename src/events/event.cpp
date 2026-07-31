#include "events/event.hpp"

#include <ostream>
#include <type_traits>

namespace moba_sim {

void process_event(const Event& event, std::ostream& debug_out) {
    // C++23: lambda używa `self` (deducing this) do rekurencyjnego
    // przetworzenia pod-zdarzeń
    std::visit(
        [&debug_out](this auto self, const auto& e) -> void {
            using T = std::decay_t<decltype(e)>;

            if constexpr (std::is_same_v<T, PlayerDiedEvent>) {
                debug_out << "Gracz zginął: " << e.name << "\n";
            } else if constexpr (std::is_same_v<T, KeyPressedEvent>) {
                debug_out << "Klawisz: " << e.key << "\n";
            } else if constexpr (std::is_same_v<T, std::shared_ptr<EventSequence>>) {
                debug_out << "--- Początek Sekwencji Zdarzeń ---\n";
                for (const auto& sub_event : e->events) {
                    // REKURENCJA! Przekazujemy 'self' (czyli tę samą lambdę)
                    // do kolejnego visit
                    std::visit(self, sub_event);
                }
                debug_out << "--- Koniec Sekwencji ---\n";
            }
        },
        event);
}

} // namespace moba_sim
