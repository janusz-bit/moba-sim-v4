#include "stats/stat_pipeline.hpp"

namespace moba_sim {

void StatPipeline::add(Modifier modifier) {
    switch (modifier.kind) {
    case ModifierKind::Base:
        base_sum_ += modifier.value;
        break;
    case ModifierKind::Inc:
        inc_sum_ += modifier.value;
        break;
    case ModifierKind::More:
        more_product_ *= (1.0 + modifier.value);
        break;
    }
}

double StatPipeline::base_total() const { return base_sum_; }

double StatPipeline::inc_multiplier() const { return 1.0 + inc_sum_; }

double StatPipeline::more_multiplier() const { return more_product_; }

double StatPipeline::compute() const { return base_sum_ * (1.0 + inc_sum_) * more_product_; }

} // namespace moba_sim