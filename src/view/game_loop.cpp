#include "view/game_loop.hpp"

#include <cstdint>

#include <SDL3/SDL.h>

namespace moba_sim::view {

GameLoop::GameLoop(std::string_view title, int width, int height, double ticks_per_second)
    : window_{title, width, height}, renderer_{window_.renderer()},
      tick_dt_{1.0 / ticks_per_second} {}

void GameLoop::run(const UpdateFn& update, const RenderFn& render) {
    running_ = true;

    constexpr double max_frame_time = 0.25; // clamp long stalls (debugger, ...)
    double accumulator = 0.0;
    std::uint64_t previous = SDL_GetTicksNS();

    while (running_) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT || event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED ||
                (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE)) {
                running_ = false;
            }
        }

        const std::uint64_t now = SDL_GetTicksNS();
        double frame_time = static_cast<double>(now - previous) / 1e9;
        previous = now;
        if (frame_time > max_frame_time) {
            frame_time = max_frame_time;
        }
        accumulator += frame_time;

        while (running_ && accumulator >= tick_dt_) {
            update(tick_dt_);
            accumulator -= tick_dt_;
        }

        if (!running_) {
            break;
        }

        render(renderer_);
        renderer_.present();
    }
}

void GameLoop::stop() { running_ = false; }

} // namespace moba_sim::view
