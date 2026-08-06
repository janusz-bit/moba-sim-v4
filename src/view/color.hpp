#pragma once

#include <cstdint>

namespace moba_sim::view {

/// RGBA color with 8-bit channels (0-255).
struct Color {
    std::uint8_t r = 0;   ///< Red channel.
    std::uint8_t g = 0;   ///< Green channel.
    std::uint8_t b = 0;   ///< Blue channel.
    std::uint8_t a = 255; ///< Alpha; opaque by default.
};

} // namespace moba_sim::view
