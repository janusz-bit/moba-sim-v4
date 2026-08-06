#pragma once

#include "view/color.hpp"
#include "view/geometry.hpp"

struct SDL_Renderer;

namespace moba_sim::view {

/// Immediate-mode 2D drawing over an SDL3 renderer: simple colored shapes in
/// pixel coordinates, y-axis pointing down. A frame starts with clear() and
/// ends with present().
class Renderer2D {
  public:
    /// Wraps an existing SDL renderer, which must outlive this object.
    explicit Renderer2D(SDL_Renderer* renderer) : renderer_{renderer} {}

    /// Fills the whole backbuffer with `color`.
    void clear(Color color);

    /// Fills the interior of `rect` with `color`.
    void fill_rect(Rect rect, Color color);

    /// Draws the 1px outline of `rect` with `color`.
    void draw_rect(Rect rect, Color color);

    /// Draws a 1px line segment with `color`.
    void draw_line(Vec2 from, Vec2 to, Color color);

    /// Fills a circle approximation (triangle fan) with `color`.
    void fill_circle(Vec2 center, float radius, Color color, int segments = 32);

    /// Pushes the drawn frame to the screen.
    void present();

  private:
    SDL_Renderer* renderer_;
};

} // namespace moba_sim::view
