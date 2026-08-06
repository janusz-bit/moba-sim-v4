#pragma once

#include <bitset>
#include <initializer_list>

#include "stats/stat_id.hpp"

namespace moba_sim {

/// A set of StatIds — the currency of effect dependency declarations.
///
/// An Effect declares which stats it reads and which it writes; EffectSet uses
/// those masks to order effects so that every effect sees final values for the
/// stats it depends on. Reading a stat that was not declared is an error, not
/// a silently wrong number (see StatView).
class StatMask {
  public:
    /// An empty mask containing no stats.
    StatMask() = default;

    /// Builds a mask from a stat list: `StatMask{StatId::AttackDamage}`.
    StatMask(std::initializer_list<StatId> stats) {
        for (StatId stat : stats) {
            bits_.set(stat_index(stat));
        }
    }

    /// A mask containing every stat. Use for effects that legitimately read
    /// everything; prefer a narrow mask, since a wide read mask constrains
    /// ordering and makes cycles more likely.
    [[nodiscard]] static StatMask all() {
        StatMask mask;
        mask.bits_.set();
        return mask;
    }

    /// Adds `stat` to the set and returns *this, so calls can chain.
    StatMask& set(StatId stat) {
        bits_.set(stat_index(stat));
        return *this;
    }

    /// True if `stat` is in the set.
    [[nodiscard]] bool contains(StatId stat) const { return bits_.test(stat_index(stat)); }
    /// True if the set holds no stats.
    [[nodiscard]] bool empty() const { return bits_.none(); }
    /// Number of stats in the set.
    [[nodiscard]] std::size_t size() const { return bits_.count(); }

    /// True if the two masks share at least one stat — i.e. one effect writes
    /// something the other reads.
    [[nodiscard]] bool intersects(const StatMask& other) const {
        return (bits_ & other.bits_).any();
    }

    /// Union of two masks.
    [[nodiscard]] StatMask operator|(const StatMask& other) const {
        StatMask result;
        result.bits_ = bits_ | other.bits_;
        return result;
    }

    /// Adds every stat of `other` to this mask.
    StatMask& operator|=(const StatMask& other) {
        bits_ |= other.bits_;
        return *this;
    }

    /// True if both masks hold exactly the same stats.
    [[nodiscard]] bool operator==(const StatMask& other) const = default;

  private:
    std::bitset<kStatCount> bits_;
};

} // namespace moba_sim
