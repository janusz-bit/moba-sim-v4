#pragma once

#include <cstdint>

namespace moba_sim::view {

/// RGBA color with 8-bit channels (0-255).
struct Color {
    std::uint8_t r = 0;
    std::uint8_t g = 0;
    std::uint8_t b = 0;
    std::uint8_t a = 255;
};

} // namespace moba_sim::view
