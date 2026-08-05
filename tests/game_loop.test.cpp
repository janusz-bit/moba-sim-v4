#include <cstdlib>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "view/game_loop.hpp"

TEST_CASE("GameLoop runs fixed-timestep updates until stopped") {
    // Headless run: use SDL's dummy video driver so no display is needed.
    setenv("SDL_VIDEO_DRIVER", "dummy", 1);

    constexpr double ticks_per_second = 1000.0;
    moba_sim::view::GameLoop loop{"test", 320, 200, ticks_per_second};

    int updates = 0;
    int renders = 0;

    loop.run(
        [&](double dt) {
            CHECK(dt == Catch::Approx(1.0 / ticks_per_second));
            ++updates;
            if (updates == 10) {
                loop.stop();
            }
        },
        [&](moba_sim::view::Renderer2D&) { ++renders; });

    CHECK(updates == 10);
    CHECK(renders >= 1);
    CHECK_FALSE(loop.running());
}
