#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace moba_sim {

/// The full provenance of one stat value: every modifier that contributed to
/// it (grouped by bucket, each labeled with its source), plus the
/// intermediate and final results of the Base/Inc/More pipeline. This is the
/// debugging answer to "where did this number come from?".
struct StatBreakdown {
    /// A single labeled contribution to a stat.
    struct Entry {
        double value = 0.0; ///< The contributed amount.
        std::string source; ///< Where the number came from, e.g. "B.F. Sword".
    };

    std::vector<Entry> base; ///< Base contributions, in application order.
    std::vector<Entry> inc;  ///< Inc contributions, in application order.
    std::vector<Entry> more; ///< More contributions, in application order.

    double base_total = 0.0;      ///< sum(Base)
    double inc_multiplier = 1.0;  ///< 1 + sum(Inc)
    double more_multiplier = 1.0; ///< product(1 + More)
    double total = 0.0;           ///< base_total * inc_multiplier * more_multiplier
};

/// Renders the breakdown as a human-readable report showing where each
/// number came from, e.g.:
///
///     AttackDamage = 130.68
///       Base = 99
///         + 59  (Ahri base, lvl 3)
///         + 40  (B.F. Sword)
///       Inc = 1.2
///         + 0.2  (Zeal)
///       99 * 1.2 = 130.68
[[nodiscard]] std::string format_breakdown(std::string_view stat_name,
                                           const StatBreakdown& breakdown);

} // namespace moba_sim
