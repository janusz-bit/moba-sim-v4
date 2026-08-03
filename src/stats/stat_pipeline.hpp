#pragma once

#include <vector>

namespace moba_sim {

/// Aggregates modifiers of one stat and computes the resulting value through
/// the Base/Inc/More pipeline:
///
///     result = sum(Base) * (1 + sum(Inc)) * product(More)
///
/// Each bucket is a separate vector; add_base / add_inc / add_more push into
/// the matching one.
class StatPipeline {
  public:
    /// Adds a value to the Base bucket (summed additively).
    void add_base(double value);
    /// Adds a value to the Inc bucket (summed, applied as 1 + sum).
    void add_inc(double value);
    /// Adds a value to the More bucket (each entry applied as 1 + value,
    /// multiplied together).
    void add_more(double value);

    /// Returns the base value before any Inc/More scaling: sum(Base).
    [[nodiscard]] double base_total() const;

    /// Returns the Inc multiplier: 1 + sum(Inc).
    [[nodiscard]] double inc_multiplier() const;

    /// Returns the product of all More multipliers (each as 1 + value).
    [[nodiscard]] double more_multiplier() const;

    /// Returns the fully computed stat value.
    [[nodiscard]] double compute() const;

  private:
    std::vector<double> base_;
    std::vector<double> inc_;
    std::vector<double> more_;
};

} // namespace moba_sim