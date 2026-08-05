#pragma once

#include <string>
#include <vector>

#include "stats/stat_breakdown.hpp"

namespace moba_sim {

/// Aggregates modifiers of one stat and computes the resulting value through
/// the Base/Inc/More pipeline:
///
///     result = sum(Base) * (1 + sum(Inc)) * product(More)
///
/// Each bucket is a separate vector; add_base / add_inc / add_more push into
/// the matching one. Every modifier carries a `source` label describing where
/// the number came from, so breakdown() can explain the final value.
class StatPipeline {
  public:
    /// Adds a value to the Base bucket (summed additively). `source` names
    /// the origin of the number (e.g. an item name) for debugging.
    void add_base(double value, std::string source = "");
    /// Adds a value to the Inc bucket (summed, applied as 1 + sum).
    void add_inc(double value, std::string source = "");
    /// Adds a value to the More bucket (each entry applied as 1 + value,
    /// multiplied together).
    void add_more(double value, std::string source = "");

    /// Returns the base value before any Inc/More scaling: sum(Base).
    [[nodiscard]] double base_total() const;

    /// Returns the Inc multiplier: 1 + sum(Inc).
    [[nodiscard]] double inc_multiplier() const;

    /// Returns the product of all More multipliers (each as 1 + value).
    [[nodiscard]] double more_multiplier() const;

    /// Returns the fully computed stat value.
    [[nodiscard]] double compute() const;

    /// Returns the full provenance of compute(): every modifier with its
    /// source label, plus the intermediate results.
    [[nodiscard]] StatBreakdown breakdown() const;

  private:
    std::vector<StatBreakdown::Entry> base_;
    std::vector<StatBreakdown::Entry> inc_;
    std::vector<StatBreakdown::Entry> more_;
};

} // namespace moba_sim