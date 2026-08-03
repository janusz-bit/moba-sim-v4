#include "stats/stat_pipeline.hpp"

namespace moba_sim {

void StatPipeline::add(const Modifier& modifier) { modifiers_.push_back(modifier); }

const std::vector<Modifier>& StatPipeline::modifiers() const { return modifiers_; }

double StatPipeline::base_total() const {
    double sum = 0.0;
    for (const auto& m : modifiers_) {
        if (m.kind == ModifierKind::Base) {
            sum += m.value;
        }
    }
    return sum;
}

double StatPipeline::inc_multiplier() const {
    double sum = 0.0;
    for (const auto& m : modifiers_) {
        if (m.kind == ModifierKind::Inc) {
            sum += m.value;
        }
    }
    return 1.0 + sum;
}

double StatPipeline::more_multiplier() const {
    double product = 1.0;
    for (const auto& m : modifiers_) {
        if (m.kind == ModifierKind::More) {
            product *= (1.0 + m.value);
        }
    }
    return product;
}

double StatPipeline::compute() const { return base_total() * inc_multiplier() * more_multiplier(); }

} // namespace moba_sim
