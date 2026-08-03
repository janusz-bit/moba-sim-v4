#include "items/item.hpp"

namespace moba_sim {

std::vector<ItemModifier> Item::modifiers_for(StatId stat) const {
    std::vector<ItemModifier> result;
    for (const auto& mod : modifiers) {
        if (mod.stat == stat) {
            result.push_back(mod);
        }
    }
    return result;
}

void Item::apply_to(Champion& champion) const {
    for (const auto& mod : modifiers) {
        champion.pipeline(mod.stat).add({mod.kind, mod.value});
    }
}

} // namespace moba_sim