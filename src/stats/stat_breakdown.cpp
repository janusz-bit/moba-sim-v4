#include "stats/stat_breakdown.hpp"

#include <array>
#include <cstdio>
#include <sstream>

namespace moba_sim {

namespace {

/// Formats a double without trailing zeros (60, 1.3, 133.848).
std::string fmt(double value) {
    std::array<char, 32> buf{};
    std::snprintf(buf.data(), buf.size(), "%.6g", value);
    return std::string{buf.data()};
}

std::string_view source_or_unknown(const std::string& source) {
    return source.empty() ? "(unknown)" : std::string_view{source};
}

} // namespace

std::string format_breakdown(std::string_view stat_name, const StatBreakdown& breakdown) {
    std::ostringstream out;
    out << stat_name << " = " << fmt(breakdown.total) << "\n";

    if (!breakdown.base.empty()) {
        out << "  Base = " << fmt(breakdown.base_total) << "\n";
        for (const auto& entry : breakdown.base) {
            out << "    + " << fmt(entry.value) << "  (" << source_or_unknown(entry.source)
                << ")\n";
        }
    }

    if (!breakdown.inc.empty()) {
        out << "  Inc = " << fmt(breakdown.inc_multiplier) << "\n";
        for (const auto& entry : breakdown.inc) {
            out << "    + " << fmt(entry.value) << "  (" << source_or_unknown(entry.source)
                << ")\n";
        }
    }

    if (!breakdown.more.empty()) {
        out << "  More = " << fmt(breakdown.more_multiplier) << "\n";
        for (const auto& entry : breakdown.more) {
            out << "    x " << fmt(1.0 + entry.value) << "  (" << source_or_unknown(entry.source)
                << ")\n";
        }
    }

    out << "  " << fmt(breakdown.base_total) << " * " << fmt(breakdown.inc_multiplier) << " * "
        << fmt(breakdown.more_multiplier) << " = " << fmt(breakdown.total);
    return out.str();
}

} // namespace moba_sim
