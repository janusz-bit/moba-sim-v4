#include <iostream>
#include <memory>

#include "events/event.hpp"

int main() {
    // Tworzymy sekwencję zdarzeń (np. makro w grze)
    auto seq = std::make_shared<moba_sim::EventSequence>();
    seq->events.push_back(moba_sim::KeyPressedEvent{32});
    seq->events.push_back(moba_sim::PlayerDiedEvent{"Anna"});

    // Wywołujemy główne zdarzenie zawierające sekwencję.
    // std::cout pełni rolę debug outputu — bez tego argumentu
    // process_event byłby cichy.
    moba_sim::Event main_event = seq;
    moba_sim::process_event(main_event, std::cout);
}
