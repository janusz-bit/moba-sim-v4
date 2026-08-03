#include "items/item.hpp"

namespace moba_sim {

void Item::apply_to(Champion& champion) const {
    for (const auto& mod : modifiers) {
        champion.pipeline(mod.stat).add({mod.kind, mod.value});
    }
}

} // namespace moba_sim