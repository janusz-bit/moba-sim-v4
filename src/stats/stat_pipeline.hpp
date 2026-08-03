#pragma once

#include <vector>

#include "stats/modifier.hpp"

namespace moba_sim {

/// Aggregates modifiers of one stat and computes the resulting value through
/// the Base/Inc/More pipeline:
///
///     result = sum(Base) * (1 + sum(Inc)) * product(More)
///
/// The three buckets are summed or multiplied as described by ModifierKind.
class StatPipeline {
  public:
    /// Adds a modifier to the matching bucket. More modifiers are stored as
    /// raw multipliers (e.g. 0.2 for +20%) and converted internally.
    void add(Modifier modifier);

    /// Returns the base value before any Inc/More scaling: sum(Base).
    [[nodiscard]] double base_total() const;

    /// Returns the Inc multiplier: 1 + sum(Inc).
    [[nodiscard]] double inc_multiplier() const;

    /// Returns the product of all More multipliers (each as 1 + value).
    [[nodiscard]] double more_multiplier() const;

    /// Returns the fully computed stat value.
    [[nodiscard]] double compute() const;

  private:
    double base_sum_ = 0.0;
    double inc_sum_ = 0.0;
    double more_product_ = 1.0;
};

} // namespace moba_sim