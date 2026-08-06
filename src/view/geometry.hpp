#pragma once

#include <cmath>

namespace moba_sim::view {

/// A 2D vector / point in screen space (x right, y down).
struct Vec2 {
    float x = 0.0f; ///< Horizontal component, growing rightwards.
    float y = 0.0f; ///< Vertical component, growing downwards.

    /// Component-wise sum.
    [[nodiscard]] constexpr Vec2 operator+(Vec2 other) const { return {x + other.x, y + other.y}; }

    /// Component-wise difference.
    [[nodiscard]] constexpr Vec2 operator-(Vec2 other) const { return {x - other.x, y - other.y}; }

    /// Scales both components by `scale`.
    [[nodiscard]] constexpr Vec2 operator*(float scale) const { return {x * scale, y * scale}; }

    /// Adds `other` in place.
    constexpr Vec2& operator+=(Vec2 other) {
        x += other.x;
        y += other.y;
        return *this;
    }

    /// Scales in place by `scale`.
    constexpr Vec2& operator*=(float scale) {
        x *= scale;
        y *= scale;
        return *this;
    }

    /// Exact component-wise equality; no epsilon.
    [[nodiscard]] constexpr bool operator==(Vec2 other) const {
        return x == other.x && y == other.y;
    }
};

/// Scales `v` by `scale`, so a scalar may appear on the left.
[[nodiscard]] constexpr Vec2 operator*(float scale, Vec2 v) { return v * scale; }

/// Returns the length of `v`.
[[nodiscard]] inline float length(Vec2 v) { return std::sqrt(v.x * v.x + v.y * v.y); }

/// Returns `v` scaled to unit length, or {0, 0} for a (near-)zero vector.
[[nodiscard]] inline Vec2 normalized(Vec2 v) {
    const float len = length(v);
    if (len < 1e-6f) {
        return {0.0f, 0.0f};
    }
    return v * (1.0f / len);
}

/// An axis-aligned rectangle given by its top-left corner and size.
struct Rect {
    float x = 0.0f; ///< Left edge.
    float y = 0.0f; ///< Top edge.
    float w = 0.0f; ///< Width, extending rightwards from x.
    float h = 0.0f; ///< Height, extending downwards from y.

    /// Returns true if `p` lies inside (or on the border of) the rectangle.
    [[nodiscard]] constexpr bool contains(Vec2 p) const {
        return p.x >= x && p.x <= x + w && p.y >= y && p.y <= y + h;
    }

    /// Returns the center point of the rectangle.
    [[nodiscard]] constexpr Vec2 center() const { return {x + w * 0.5f, y + h * 0.5f}; }
};

} // namespace moba_sim::view
