#pragma once

#include <SDL2/SDL.h>

#include <SDL2/SDL_opengl.h>
#include <SDL2/SDL_opengl_glext.h>

#include <string>

class Map;
class Player;

struct Light {
    float x;
    float y;
    float radius;
    float intensity;
    float r;
    float g;
    float b;
    float falloffExponent;
};

class Renderer {
public:
    bool initialize(SDL_Window* window);
    void shutdown();
    void setAmbient(float value);
    void setGlobalTint(float r, float g, float b);
    void render(const Map& map, const Player& player, const Light& playerLight, const Light& lampLight);

private:
    bool loadGlFunctions();
    bool initializeGpuPipeline();
    void destroyGpuPipeline();
    bool ensureRenderTargets();

    bool loadShaderSource(const char* path, std::string& outSource) const;
    GLuint compileShader(GLenum shaderType, const char* source, const char* label) const;
    GLuint linkProgram(GLuint vertexShader, GLuint fragmentShader, const char* label) const;

    void renderCpuLighting(const Map& map, const Player& player, const Light& playerLight, const Light& lampLight) const;
    void renderSceneAlbedo(const Map& map, const Player& player) const;
    void drawFullscreenQuad() const;

    SDL_Window* m_window = nullptr;
    SDL_GLContext m_context = nullptr;
    float m_ambient = 0.35F;
    float m_globalTintR = 1.0F;
    float m_globalTintG = 1.0F;
    float m_globalTintB = 1.0F;
    bool m_forceCpuPath = false;

    int m_targetWidth = 0;
    int m_targetHeight = 0;

    GLuint m_albedoProgram = 0;
    GLuint m_lightProgram = 0;
    GLuint m_compositeProgram = 0;

    GLuint m_albedoFbo = 0;
    GLuint m_albedoTex = 0;
    GLuint m_lightFbo = 0;
    GLuint m_lightTex = 0;
};
