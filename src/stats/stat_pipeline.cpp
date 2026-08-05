#include "stats/stat_pipeline.hpp"

#include <utility>

namespace moba_sim {

void StatPipeline::add_base(double value, std::string source) {
    base_.push_back({value, std::move(source)});
}

void StatPipeline::add_inc(double value, std::string source) {
    inc_.push_back({value, std::move(source)});
}

void StatPipeline::add_more(double value, std::string source) {
    more_.push_back({value, std::move(source)});
}

double StatPipeline::base_total() const {
    double sum = 0.0;
    for (const auto& entry : base_) {
        sum += entry.value;
    }
    return sum;
}

double StatPipeline::inc_multiplier() const {
    double sum = 0.0;
    for (const auto& entry : inc_) {
        sum += entry.value;
    }
    return 1.0 + sum;
}

double StatPipeline::more_multiplier() const {
    double product = 1.0;
    for (const auto& entry : more_) {
        product *= (1.0 + entry.value);
    }
    return product;
}

double StatPipeline::compute() const { return base_total() * inc_multiplier() * more_multiplier(); }

StatBreakdown StatPipeline::breakdown() const {
    StatBreakdown result;
    result.base = base_;
    result.inc = inc_;
    result.more = more_;
    result.base_total = base_total();
    result.inc_multiplier = inc_multiplier();
    result.more_multiplier = more_multiplier();
    result.total = compute();
    return result;
}

} // namespace moba_sim
