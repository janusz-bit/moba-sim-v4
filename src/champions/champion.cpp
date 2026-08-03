#include "champions/champion.hpp"

namespace moba_sim {

double Champion::base_value(StatId stat, int level) const {
    const auto [base_ptr, growth_ptr] = spec_for(stat);
    if (base_ptr == nullptr) {
        return 0.0;
    }
    const int growth_steps = level > 1 ? level - 1 : 0;
    return (this->*base_ptr) + (this->*growth_ptr) * growth_steps;
}

StatPipeline Champion::pipeline_for(StatId stat, int level) const {
    StatPipeline pipeline;
    pipeline.add({ModifierKind::Base, base_value(stat, level)});
    return pipeline;
}

} // namespace moba_sim