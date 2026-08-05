#include "view/renderer2d.hpp"

#include <vector>

#include <SDL3/SDL.h>

namespace moba_sim::view {

namespace {

SDL_FColor to_sdl(Color c) {
    return SDL_FColor{
        static_cast<float>(c.r) / 255.0f,
        static_cast<float>(c.g) / 255.0f,
        static_cast<float>(c.b) / 255.0f,
        static_cast<float>(c.a) / 255.0f,
    };
}

} // namespace

void Renderer2D::clear(Color color) {
    (void)SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a);
    (void)SDL_RenderClear(renderer_);
}

void Renderer2D::fill_rect(Rect rect, Color color) {
    (void)SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a);
    const SDL_FRect sdl_rect{rect.x, rect.y, rect.w, rect.h};
    (void)SDL_RenderFillRect(renderer_, &sdl_rect);
}

void Renderer2D::draw_rect(Rect rect, Color color) {
    (void)SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a);
    const SDL_FRect sdl_rect{rect.x, rect.y, rect.w, rect.h};
    (void)SDL_RenderRect(renderer_, &sdl_rect);
}

void Renderer2D::draw_line(Vec2 from, Vec2 to, Color color) {
    (void)SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a);
    (void)SDL_RenderLine(renderer_, from.x, from.y, to.x, to.y);
}

void Renderer2D::fill_circle(Vec2 center, float radius, Color color, int segments) {
    if (segments < 3) {
        segments = 3;
    }

    const SDL_FColor sdl_color = to_sdl(color);

    // Triangle fan: vertex 0 is the center, the ring vertices follow.
    std::vector<SDL_Vertex> vertices;
    vertices.reserve(static_cast<std::size_t>(segments) + 2);
    vertices.push_back(SDL_Vertex{{center.x, center.y}, sdl_color, {0.0f, 0.0f}});
    for (int i = 0; i <= segments; ++i) {
        const float angle =
            2.0f * 3.14159265f * static_cast<float>(i) / static_cast<float>(segments);
        vertices.push_back(
            SDL_Vertex{{center.x + radius * std::cos(angle), center.y + radius * std::sin(angle)},
                       sdl_color,
                       {0.0f, 0.0f}});
    }

    std::vector<int> indices;
    indices.reserve(static_cast<std::size_t>(segments) * 3);
    for (int i = 0; i < segments; ++i) {
        indices.push_back(0);
        indices.push_back(i + 1);
        indices.push_back(i + 2);
    }

    (void)SDL_RenderGeometry(renderer_, nullptr, vertices.data(), static_cast<int>(vertices.size()),
                             indices.data(), static_cast<int>(indices.size()));
}

void Renderer2D::present() { (void)SDL_RenderPresent(renderer_); }

} // namespace moba_sim::view
