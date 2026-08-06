#pragma once

#include <array>

#include "stats/modifier.hpp"
#include "stats/stat_breakdown.hpp"
#include "stats/stat_id.hpp"
#include "stats/stat_pipeline.hpp"

namespace moba_sim {

/// One StatPipeline per StatId: the complete stat state of a unit.
///
/// This is what effects read from and contribute into. It exists as its own
/// type (rather than an array buried inside Champion) so the effect layer can
/// operate on stats without knowing what owns them.
class StatTable {
  public:
    /// Mutable pipeline for `stat`, for seeding and contributing.
    [[nodiscard]] StatPipeline& operator[](StatId stat) { return pipelines_[stat_index(stat)]; }
    /// Read-only pipeline for `stat`.
    [[nodiscard]] const StatPipeline& operator[](StatId stat) const {
        return pipelines_[stat_index(stat)];
    }

    /// Fully computed value of `stat`: sum(Base) * (1 + sum(Inc)) * product(More).
    [[nodiscard]] double compute(StatId stat) const { return (*this)[stat].compute(); }

    /// Provenance of compute(stat): every contributing modifier with its label.
    [[nodiscard]] StatBreakdown breakdown(StatId stat) const { return (*this)[stat].breakdown(); }

    /// Pushes `mod` into the right bucket of the right stat.
    void apply(const Modifier& mod) { apply_modifier((*this)[mod.stat], mod); }

    /// Same, with the provenance label supplied by the caller.
    void apply(const Modifier& mod, const std::string& source) {
        apply_modifier((*this)[mod.stat], mod, source);
    }

    /// Drops every modifier from every stat, leaving all values at 0.
    void clear() { pipelines_ = {}; }

  private:
    std::array<StatPipeline, kStatCount> pipelines_;
};

} // namespace moba_sim
