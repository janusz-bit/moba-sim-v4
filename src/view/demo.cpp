#include <cmath>
#include <vector>

#include <SDL3/SDL.h>

#include "view/game_loop.hpp"

using moba_sim::view::Color;
using moba_sim::view::Rect;
using moba_sim::view::Vec2;

namespace {

struct Unit {
    Vec2 pos;
    Vec2 vel;
    float radius = 16.0f;
    Color color;
};

/// Reflects `unit` off the arena borders so it stays inside.
void bounce_inside(Unit& unit, float arena_w, float arena_h) {
    if (unit.pos.x - unit.radius < 0.0f) {
        unit.pos.x = unit.radius;
        unit.vel.x = std::abs(unit.vel.x);
    } else if (unit.pos.x + unit.radius > arena_w) {
        unit.pos.x = arena_w - unit.radius;
        unit.vel.x = -std::abs(unit.vel.x);
    }

    if (unit.pos.y - unit.radius < 0.0f) {
        unit.pos.y = unit.radius;
        unit.vel.y = std::abs(unit.vel.y);
    } else if (unit.pos.y + unit.radius > arena_h) {
        unit.pos.y = arena_h - unit.radius;
        unit.vel.y = -std::abs(unit.vel.y);
    }
}

/// Clamps `pos` so a circle of `radius` stays inside the arena.
Vec2 clamp_to_arena(Vec2 pos, float radius, float arena_w, float arena_h) {
    pos.x = std::max(radius, std::min(arena_w - radius, pos.x));
    pos.y = std::max(radius, std::min(arena_h - radius, pos.y));
    return pos;
}

} // namespace

int main() {
    moba_sim::view::GameLoop loop{"moba-sim view", 1024, 768, 60.0};

    Unit player{{512.0f, 384.0f}, {0.0f, 0.0f}, 18.0f, {90, 160, 255}};

    std::vector<Unit> npcs{
        {{200.0f, 150.0f}, {140.0f, 90.0f}, 14.0f, {230, 90, 90}},
        {{820.0f, 600.0f}, {-110.0f, -140.0f}, 14.0f, {240, 190, 80}},
        {{700.0f, 200.0f}, {-90.0f, 120.0f}, 12.0f, {140, 220, 120}},
        {{300.0f, 550.0f}, {160.0f, -80.0f}, 12.0f, {200, 130, 220}},
    };

    const std::vector<Rect> obstacles{
        {420.0f, 180.0f, 180.0f, 40.0f},
        {420.0f, 548.0f, 180.0f, 40.0f},
        {180.0f, 340.0f, 40.0f, 120.0f},
        {804.0f, 340.0f, 40.0f, 120.0f},
    };

    loop.run(
        [&](double dt) {
            const float fdt = static_cast<float>(dt);
            const float arena_w = static_cast<float>(loop.width());
            const float arena_h = static_cast<float>(loop.height());

            // Player movement from WASD / arrow keys.
            const bool* keys = SDL_GetKeyboardState(nullptr);
            Vec2 dir{};
            if (keys[SDL_SCANCODE_W] || keys[SDL_SCANCODE_UP]) {
                dir.y -= 1.0f;
            }
            if (keys[SDL_SCANCODE_S] || keys[SDL_SCANCODE_DOWN]) {
                dir.y += 1.0f;
            }
            if (keys[SDL_SCANCODE_A] || keys[SDL_SCANCODE_LEFT]) {
                dir.x -= 1.0f;
            }
            if (keys[SDL_SCANCODE_D] || keys[SDL_SCANCODE_RIGHT]) {
                dir.x += 1.0f;
            }

            constexpr float player_speed = 280.0f; // pixels per second
            player.vel = normalized(dir) * player_speed;
            player.pos =
                clamp_to_arena(player.pos + player.vel * fdt, player.radius, arena_w, arena_h);

            // NPCs wander and bounce off the arena borders.
            for (Unit& npc : npcs) {
                npc.pos += npc.vel * fdt;
                bounce_inside(npc, arena_w, arena_h);
            }
        },
        [&](moba_sim::view::Renderer2D& r) {
            r.clear({22, 26, 22});
            r.fill_rect(
                {0.0f, 0.0f, static_cast<float>(loop.width()), static_cast<float>(loop.height())},
                {38, 64, 38});

            for (const Rect& obstacle : obstacles) {
                r.fill_rect(obstacle, {96, 96, 104});
                r.draw_rect(obstacle, {140, 140, 150});
            }

            for (const Unit& npc : npcs) {
                r.fill_circle(npc.pos, npc.radius, npc.color);
            }

            r.fill_circle(player.pos, player.radius, player.color);

            // A placeholder health bar above the player.
            const Rect bar{player.pos.x - player.radius, player.pos.y - player.radius - 14.0f,
                           2.0f * player.radius, 6.0f};
            r.fill_rect(bar, {20, 20, 20});
            r.fill_rect({bar.x + 1.0f, bar.y + 1.0f, 0.7f * (bar.w - 2.0f), bar.h - 2.0f},
                        {90, 220, 90});
        });
}
