#pragma once

#include <string_view>

struct SDL_Renderer;
struct SDL_Window;

namespace moba_sim::view {

/// RAII wrapper over an SDL3 window and its 2D renderer. Initializes the SDL
/// video subsystem on construction and shuts it down on destruction. Throws
/// std::runtime_error if initialization fails.
class Window {
  public:
    /// Opens a window of `width` x `height` titled `title`, initialising SDL
    /// video. Throws std::runtime_error if that fails.
    Window(std::string_view title, int width, int height);
    /// Releases the window and renderer and shuts down SDL video.
    ~Window();

    /// Non-copyable: the window is a unique resource.
    Window(const Window&) = delete;
    /// Non-copyable: the window is a unique resource.
    Window& operator=(const Window&) = delete;
    /// Non-movable: the window is a unique resource.
    Window(Window&&) = delete;
    /// Non-movable: the window is a unique resource.
    Window& operator=(Window&&) = delete;

    /// Returns the underlying SDL renderer.
    [[nodiscard]] SDL_Renderer* renderer() const { return renderer_; }

    /// Returns the current window size in pixels.
    [[nodiscard]] int width() const;
    /// Current window height in pixels.
    [[nodiscard]] int height() const;

  private:
    SDL_Window* window_ = nullptr;
    SDL_Renderer* renderer_ = nullptr;
};

} // namespace moba_sim::view
