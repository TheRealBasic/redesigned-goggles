#include "core/Application.hpp"

#include "core/Timer.hpp"
#include "game/Map.hpp"
#include "game/Player.hpp"
#include "render/Renderer.hpp"

#include <SDL2/SDL.h>

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace {
constexpr float kTau = 6.28318530718F;

float smoothstep(float edge0, float edge1, float x) {
    const float t = std::clamp((x - edge0) / (edge1 - edge0), 0.0F, 1.0F);
    return t * t * (3.0F - 2.0F * t);
}

float mix(float a, float b, float t) {
    return a + (b - a) * t;
}
}

bool Application::run() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        return false;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    SDL_Window* window = SDL_CreateWindow(
        "Western RPG Prototype - Phase 1",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        3840,
        2160,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

    if (window == nullptr) {
        SDL_Quit();
        return false;
    }

    Renderer renderer;
    if (!renderer.initialize(window)) {
        SDL_DestroyWindow(window);
        SDL_Quit();
        return false;
    }

    Map map;
    if (!map.loadFromAsciiFile("data/maps/frontier_town.map")) {
        renderer.shutdown();
        SDL_DestroyWindow(window);
        SDL_Quit();
        return false;
    }

    Player player;
    player.setPosition(2.5F, 2.5F);

    Timer timer;
    bool running = true;
    std::uint64_t previous = SDL_GetPerformanceCounter();

    Light playerLight{player.x(), player.y(), 4.2F, 0.88F, 1.00F, 0.78F, 0.52F, 2.3F};
    Light lampLight{11.0F, 7.0F, 4.0F, 0.72F, 1.00F, 0.70F, 0.42F, 1.8F};

    float worldTime = 0.0F;

    while (running) {
        const std::uint64_t current = SDL_GetPerformanceCounter();
        const std::uint64_t freq = SDL_GetPerformanceFrequency();
        const double frameSeconds = static_cast<double>(current - previous) / static_cast<double>(freq);
        previous = current;

        InputState input{};
        SDL_Event event;
        while (SDL_PollEvent(&event) == 1) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
            if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
                running = false;
            }
        }

        const Uint8* keys = SDL_GetKeyboardState(nullptr);
        input.up = keys[SDL_SCANCODE_W] != 0 || keys[SDL_SCANCODE_UP] != 0;
        input.down = keys[SDL_SCANCODE_S] != 0 || keys[SDL_SCANCODE_DOWN] != 0;
        input.left = keys[SDL_SCANCODE_A] != 0 || keys[SDL_SCANCODE_LEFT] != 0;
        input.right = keys[SDL_SCANCODE_D] != 0 || keys[SDL_SCANCODE_RIGHT] != 0;

        timer.tick(std::min(frameSeconds, 0.1));
        while (timer.canStep()) {
            const float dt = static_cast<float>(timer.delta());
            worldTime += dt;

            player.update(input, map, dt);
            playerLight.x = player.x();
            playerLight.y = player.y();

            constexpr float kDayLengthSeconds = 72.0F;
            const float dayPhase = std::fmod(worldTime / kDayLengthSeconds, 1.0F);

            const float dawn = smoothstep(0.20F, 0.32F, dayPhase);
            const float dusk = smoothstep(0.68F, 0.82F, dayPhase);
            const float daylight = std::clamp(dawn - dusk, 0.0F, 1.0F);
            const float twilight = std::clamp((dawn * (1.0F - daylight)) + (dusk * (1.0F - daylight)), 0.0F, 1.0F);

            const float ambient = mix(0.11F, 0.38F, daylight) + 0.06F * twilight;
            renderer.setAmbient(ambient);

            const float nightTintR = 0.72F;
            const float nightTintG = 0.82F;
            const float nightTintB = 1.05F;
            const float duskTintR = 1.12F;
            const float duskTintG = 0.95F;
            const float duskTintB = 0.82F;

            const float baseTintR = mix(nightTintR, 1.0F, daylight);
            const float baseTintG = mix(nightTintG, 1.0F, daylight);
            const float baseTintB = mix(nightTintB, 1.0F, daylight);

            const float tintR = mix(baseTintR, duskTintR, twilight);
            const float tintG = mix(baseTintG, duskTintG, twilight);
            const float tintB = mix(baseTintB, duskTintB, twilight);
            renderer.setGlobalTint(tintR, tintG, tintB);

            const float playerFlicker = 0.93F + 0.07F * std::sin(worldTime * 14.0F + 1.1F);
            const float lampFlicker = 0.9F + 0.1F * std::sin(worldTime * 9.0F + 0.3F) * std::sin(worldTime * 5.0F + 0.8F);
            playerLight.intensity = 0.82F * playerFlicker;
            lampLight.intensity = 0.66F * lampFlicker;

            playerLight.radius = 3.9F + 0.25F * std::sin(worldTime * 3.5F);
            lampLight.radius = 3.6F + 0.45F * (0.5F + 0.5F * std::sin(worldTime * 2.1F + kTau * 0.25F));

            timer.consumeStep();
        }

        renderer.render(map, player, playerLight, lampLight);
    }

    renderer.shutdown();
    SDL_DestroyWindow(window);
    SDL_Quit();
    return true;
}
