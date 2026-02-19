#pragma once

#include <SDL2/SDL.h>

class Map;
class Player;

struct Light {
    float x;
    float y;
    float radius;
    float intensity;
};

class Renderer {
public:
    bool initialize(SDL_Window* window);
    void shutdown();
    void setAmbient(float value);
    void render(const Map& map, const Player& player, const Light& playerLight, const Light& lampLight);

private:
    SDL_Window* m_window = nullptr;
    SDL_GLContext m_context = nullptr;
    float m_ambient = 0.35F;
};
