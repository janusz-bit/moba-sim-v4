#pragma once

#include <functional>
#include <string_view>

#include "view/renderer2d.hpp"
#include "view/window.hpp"

namespace moba_sim::view {

/// A fixed-timestep game loop ("Fix Your Timestep" style):
///
///   * update(dt) is called zero or more times per frame with a constant dt
///     (1 / ticks_per_second) — this is where the game simulation runs;
///   * render() is called once per frame to draw the current state.
///
/// The loop owns the window and stops when stop() is called, the window is
/// closed, or Escape is pressed.
class GameLoop {
  public:
    using UpdateFn = std::function<void(double dt)>;
    using RenderFn = std::function<void(Renderer2D& renderer)>;

    /// Creates the window and renderer. `ticks_per_second` is the fixed
    /// simulation rate (must be > 0).
    GameLoop(std::string_view title, int width, int height, double ticks_per_second = 60.0);

    /// Runs until stop() is requested or the window is closed.
    void run(const UpdateFn& update, const RenderFn& render);

    /// Requests the loop to stop; safe to call from update/render callbacks.
    void stop();

    /// Returns true while the loop is running.
    [[nodiscard]] bool running() const { return running_; }

    /// Current window size in pixels (useful for keeping the simulation in
    /// bounds).
    [[nodiscard]] int width() const { return window_.width(); }
    [[nodiscard]] int height() const { return window_.height(); }

  private:
    Window window_;
    Renderer2D renderer_;
    double tick_dt_;
    bool running_ = false;
};

} // namespace moba_sim::view
