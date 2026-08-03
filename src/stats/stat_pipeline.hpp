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
    /// Adds a modifier to the pipeline.
    void add(const Modifier& modifier);

    /// Returns all modifiers added so far.
    [[nodiscard]] const std::vector<Modifier>& modifiers() const;

    /// Returns the base value before any Inc/More scaling: sum(Base).
    [[nodiscard]] double base_total() const;

    /// Returns the Inc multiplier: 1 + sum(Inc).
    [[nodiscard]] double inc_multiplier() const;

    /// Returns the product of all More multipliers (each as 1 + value).
    [[nodiscard]] double more_multiplier() const;

    /// Returns the fully computed stat value.
    [[nodiscard]] double compute() const;

  private:
    std::vector<Modifier> modifiers_;
};

} // namespace moba_sim
