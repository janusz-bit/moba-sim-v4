#include "view/window.hpp"

#include <stdexcept>
#include <string>

#include <SDL3/SDL.h>

namespace moba_sim::view {

Window::Window(std::string_view title, int width, int height) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        throw std::runtime_error{"SDL_Init failed: " + std::string{SDL_GetError()}};
    }

    const std::string title_str{title};
    if (!SDL_CreateWindowAndRenderer(title_str.c_str(), width, height, 0, &window_, &renderer_)) {
        SDL_Quit();
        throw std::runtime_error{"SDL_CreateWindowAndRenderer failed: " +
                                 std::string{SDL_GetError()}};
    }

    // Sync presentation with the display refresh rate; fall back to
    // unsynced rendering if the renderer does not support it.
    (void)SDL_SetRenderVSync(renderer_, 1);
}

Window::~Window() {
    if (renderer_ != nullptr) {
        SDL_DestroyRenderer(renderer_);
    }
    if (window_ != nullptr) {
        SDL_DestroyWindow(window_);
    }
    SDL_Quit();
}

int Window::width() const {
    int w = 0;
    (void)SDL_GetWindowSize(window_, &w, nullptr);
    return w;
}

int Window::height() const {
    int h = 0;
    (void)SDL_GetWindowSize(window_, nullptr, &h);
    return h;
}

} // namespace moba_sim::view
