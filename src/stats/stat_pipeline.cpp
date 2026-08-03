#include "stats/stat_pipeline.hpp"

namespace moba_sim {

void StatPipeline::add_base(double value) { base_.push_back(value); }

void StatPipeline::add_inc(double value) { inc_.push_back(value); }

void StatPipeline::add_more(double value) { more_.push_back(value); }

double StatPipeline::base_total() const {
    double sum = 0.0;
    for (const auto v : base_) {
        sum += v;
    }
    return sum;
}

double StatPipeline::inc_multiplier() const {
    double sum = 0.0;
    for (const auto v : inc_) {
        sum += v;
    }
    return 1.0 + sum;
}

double StatPipeline::more_multiplier() const {
    double product = 1.0;
    for (const auto v : more_) {
        product *= (1.0 + v);
    }
    return product;
}

double StatPipeline::compute() const { return base_total() * inc_multiplier() * more_multiplier(); }

} // namespace moba_sim