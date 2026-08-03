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

} // namespace moba_sim