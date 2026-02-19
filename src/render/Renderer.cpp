#include "render/Renderer.hpp"

#include "game/Map.hpp"
#include "game/Player.hpp"

#include <SDL2/SDL_opengl.h>

namespace {
float pseudoLight(float tileX, float tileY, const Light& light) {
    const float dx = tileX - light.x;
    const float dy = tileY - light.y;
    const float distSq = dx * dx + dy * dy;
    const float radiusSq = light.radius * light.radius;
    if (distSq >= radiusSq) {
        return 0.0F;
    }
    const float attenuation = 1.0F - (distSq / radiusSq);
    return attenuation * light.intensity;
}
} // namespace

bool Renderer::initialize(SDL_Window* window) {
    m_window = window;
    m_context = SDL_GL_CreateContext(window);
    return m_context != nullptr;
}

void Renderer::shutdown() {
    if (m_context != nullptr) {
        SDL_GL_DeleteContext(m_context);
        m_context = nullptr;
    }
}

void Renderer::setAmbient(float value) {
    m_ambient = value;
}

void Renderer::render(const Map& map, const Player& player, const Light& playerLight, const Light& lampLight) {
    glClearColor(0.06F, 0.06F, 0.08F, 1.0F);
    glClear(GL_COLOR_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, 1280.0, 720.0, 0.0, -1.0, 1.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    constexpr float tileW = 64.0F;
    constexpr float tileH = 32.0F;
    constexpr float originX = 640.0F;
    constexpr float originY = 120.0F;

    glBegin(GL_QUADS);
    for (int y = 0; y < map.height(); ++y) {
        for (int x = 0; x < map.width(); ++x) {
            const bool blocked = map.isBlocked(x, y);
            const float sx = originX + (x - y) * (tileW * 0.5F);
            const float sy = originY + (x + y) * (tileH * 0.5F);

            float light = m_ambient;
            light += pseudoLight(static_cast<float>(x), static_cast<float>(y), playerLight);
            light += pseudoLight(static_cast<float>(x), static_cast<float>(y), lampLight);
            if (light > 1.0F) {
                light = 1.0F;
            }

            if (blocked) {
                glColor3f(0.42F * light, 0.30F * light, 0.20F * light);
            } else {
                glColor3f(0.67F * light, 0.59F * light, 0.34F * light);
            }

            glVertex2f(sx, sy + tileH * 0.5F);
            glVertex2f(sx + tileW * 0.5F, sy);
            glVertex2f(sx + tileW, sy + tileH * 0.5F);
            glVertex2f(sx + tileW * 0.5F, sy + tileH);
        }
    }
    glEnd();

    const float playerSx = originX + (player.x() - player.y()) * (tileW * 0.5F) + tileW * 0.5F;
    const float playerSy = originY + (player.x() + player.y()) * (tileH * 0.5F) + tileH * 0.5F;

    glColor3f(0.2F, 0.4F, 0.85F);
    glBegin(GL_QUADS);
    glVertex2f(playerSx - 8.0F, playerSy - 20.0F);
    glVertex2f(playerSx + 8.0F, playerSy - 20.0F);
    glVertex2f(playerSx + 8.0F, playerSy);
    glVertex2f(playerSx - 8.0F, playerSy);
    glEnd();

    SDL_GL_SwapWindow(m_window);
}
