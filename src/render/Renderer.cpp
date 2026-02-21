#include "render/Renderer.hpp"

#include "game/Map.hpp"
#include "game/Player.hpp"

#include <algorithm>
#include <cstdint>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <vector>
#include <type_traits>
#include <iostream>

namespace fs = std::filesystem;

namespace {

struct GlFunctions {
    PFNGLACTIVETEXTUREPROC activeTexture = nullptr;
    PFNGLATTACHSHADERPROC attachShader = nullptr;
    PFNGLCOMPILESHADERPROC compileShader = nullptr;
    PFNGLCREATEPROGRAMPROC createProgram = nullptr;
    PFNGLCREATESHADERPROC createShader = nullptr;
    PFNGLDELETEPROGRAMPROC deleteProgram = nullptr;
    PFNGLDELETESHADERPROC deleteShader = nullptr;
    PFNGLGETPROGRAMIVPROC getProgramiv = nullptr;
    PFNGLGETPROGRAMINFOLOGPROC getProgramInfoLog = nullptr;
    PFNGLGETSHADERIVPROC getShaderiv = nullptr;
    PFNGLGETSHADERINFOLOGPROC getShaderInfoLog = nullptr;
    PFNGLGETUNIFORMLOCATIONPROC getUniformLocation = nullptr;
    PFNGLLINKPROGRAMPROC linkProgram = nullptr;
    PFNGLSHADERSOURCEPROC shaderSource = nullptr;
    PFNGLUSEPROGRAMPROC useProgram = nullptr;
    PFNGLUNIFORM1FPROC uniform1f = nullptr;
    PFNGLUNIFORM2FPROC uniform2f = nullptr;
    PFNGLUNIFORM3FPROC uniform3f = nullptr;
    PFNGLUNIFORM4FPROC uniform4f = nullptr;
    PFNGLUNIFORM1IPROC uniform1i = nullptr;
    PFNGLBINDFRAMEBUFFERPROC bindFramebuffer = nullptr;
    PFNGLDELETEFRAMEBUFFERSPROC deleteFramebuffers = nullptr;
    PFNGLGENFRAMEBUFFERSPROC genFramebuffers = nullptr;
    PFNGLCHECKFRAMEBUFFERSTATUSPROC checkFramebufferStatus = nullptr;
    PFNGLFRAMEBUFFERTEXTURE2DPROC framebufferTexture2D = nullptr;
};

GlFunctions g_gl;

#define glActiveTexture g_gl.activeTexture
#define glAttachShader g_gl.attachShader
#define glCompileShader g_gl.compileShader
#define glCreateProgram g_gl.createProgram
#define glCreateShader g_gl.createShader
#define glDeleteProgram g_gl.deleteProgram
#define glDeleteShader g_gl.deleteShader
#define glGetProgramiv g_gl.getProgramiv
#define glGetProgramInfoLog g_gl.getProgramInfoLog
#define glGetShaderiv g_gl.getShaderiv
#define glGetShaderInfoLog g_gl.getShaderInfoLog
#define glGetUniformLocation g_gl.getUniformLocation
#define glLinkProgram g_gl.linkProgram
#define glShaderSource g_gl.shaderSource
#define glUseProgram g_gl.useProgram
#define glUniform1f g_gl.uniform1f
#define glUniform2f g_gl.uniform2f
#define glUniform3f g_gl.uniform3f
#define glUniform4f g_gl.uniform4f
#define glUniform1i g_gl.uniform1i
#define glBindFramebuffer g_gl.bindFramebuffer
#define glDeleteFramebuffers g_gl.deleteFramebuffers
#define glGenFramebuffers g_gl.genFramebuffers
#define glCheckFramebufferStatus g_gl.checkFramebufferStatus
#define glFramebufferTexture2D g_gl.framebufferTexture2D

constexpr float kTileW = 64.0F;
constexpr float kTileH = 32.0F;
constexpr float kOriginX = 640.0F;
constexpr float kOriginY = 120.0F;

constexpr char kFullscreenVertexShader[] = R"(
#version 120

void main() {
    gl_Position = ftransform();
    gl_TexCoord[0] = gl_MultiTexCoord0;
}
)";

constexpr char kUseGpuLightingEnv[] = "RENDERER_FORCE_CPU_LIGHTING";

fs::path resolveResourcePath(const fs::path& relativePath) {
    auto findFromRoot = [&](const fs::path& root) -> fs::path {
        std::error_code ec;
        fs::path cursor = root;

        while (!cursor.empty()) {
            const fs::path candidate = cursor / relativePath;
            if (fs::exists(candidate, ec) && !ec) {
                return candidate;
            }

            const fs::path parent = cursor.parent_path();
            if (parent == cursor) {
                break;
            }
            cursor = parent;
        }

        return {};
    };

    if (const fs::path fromCwd = findFromRoot(fs::current_path()); !fromCwd.empty()) {
        return fromCwd;
    }

    if (const char* basePath = SDL_GetBasePath(); basePath != nullptr) {
        const fs::path fromBasePath = findFromRoot(fs::path(basePath));
        SDL_free(const_cast<char*>(basePath));
        if (!fromBasePath.empty()) {
            return fromBasePath;
        }
    }

    return relativePath;
}

float pseudoLight(float tileX, float tileY, const Light& light) {
    const float dx = tileX - light.x;
    const float dy = tileY - light.y;
    const float dist = std::sqrt(dx * dx + dy * dy);
    const float radius = std::max(0.001F, light.radius);
    const float normalized = dist / radius;

    float attenuation = 0.0F;
    if (light.falloffExponent > 0.0F) {
        attenuation = std::pow(std::clamp(1.0F - normalized, 0.0F, 1.0F), light.falloffExponent);
    } else {
        const float k = 1.0F / (radius * radius);
        attenuation = 1.0F / (1.0F + k * dist * dist);
    }

    return attenuation * light.intensity;
}

struct OcclusionCache {
    int width = 0;
    int height = 0;
    std::vector<int8_t> values;

    explicit OcclusionCache(int w, int h) : width(w), height(h), values(static_cast<size_t>(w * h), static_cast<int8_t>(-1)) {}

    int index(int x, int y) const {
        return y * width + x;
    }

    int8_t get(int x, int y) const {
        return values[static_cast<size_t>(index(x, y))];
    }

    void set(int x, int y, int8_t value) {
        values[static_cast<size_t>(index(x, y))] = value;
    }
};

bool hasLineOcclusion(const Map& map, int fromX, int fromY, int toX, int toY) {
    int x = fromX;
    int y = fromY;

    const int dx = std::abs(toX - fromX);
    const int sx = fromX < toX ? 1 : -1;
    const int dy = -std::abs(toY - fromY);
    const int sy = fromY < toY ? 1 : -1;
    int err = dx + dy;

    while (!(x == toX && y == toY)) {
        const int twiceErr = err * 2;
        if (twiceErr >= dy) {
            err += dy;
            x += sx;
        }
        if (twiceErr <= dx) {
            err += dx;
            y += sy;
        }

        if (x == toX && y == toY) {
            break;
        }

        if (map.isBlocked(x, y)) {
            return true;
        }
    }

    return false;
}

float directWithOcclusion(const Map& map, int tileX, int tileY, const Light& light, OcclusionCache& cache) {
    const int cacheState = cache.get(tileX, tileY);
    bool occluded = false;
    if (cacheState >= 0) {
        occluded = cacheState == 1;
    } else {
        const int lightTileX = static_cast<int>(std::round(light.x));
        const int lightTileY = static_cast<int>(std::round(light.y));
        occluded = hasLineOcclusion(map, lightTileX, lightTileY, tileX, tileY);
        cache.set(tileX, tileY, static_cast<int8_t>(occluded ? 1 : 0));
    }

    constexpr float kOccludedDirectScale = 0.12F;
    const float direct = pseudoLight(static_cast<float>(tileX), static_cast<float>(tileY), light);
    return direct * (occluded ? kOccludedDirectScale : 1.0F);
}
} // namespace

bool Renderer::initialize(SDL_Window* window) {
    m_window = window;
    m_context = SDL_GL_CreateContext(window);
    if (m_context == nullptr) {
        std::cerr << "SDL_GL_CreateContext failed: " << SDL_GetError() << '\n';
        return false;
    }

    const char* forceCpu = std::getenv(kUseGpuLightingEnv);
    if (!loadGlFunctions()) {
        std::cerr << "Required OpenGL entry points are unavailable; falling back to CPU lighting path.\n";
        m_forceCpuPath = true;
        return true;
    }
    m_forceCpuPath = forceCpu != nullptr && forceCpu[0] != '\0' && forceCpu[0] != '0';

    if (m_forceCpuPath) {
        std::cerr << "GPU lighting disabled by " << kUseGpuLightingEnv << "; using CPU lighting path.\n";
    }

    if (!m_forceCpuPath && !initializeGpuPipeline()) {
        std::cerr << "GPU lighting pipeline init failed; falling back to CPU lighting path.\n";
        m_forceCpuPath = true;
    }

    return true;
}

void Renderer::shutdown() {
    destroyGpuPipeline();

    if (m_context != nullptr) {
        SDL_GL_DeleteContext(m_context);
        m_context = nullptr;
    }
}

void Renderer::setAmbient(float value) {
    m_ambient = std::clamp(value, 0.0F, 1.0F);
}

void Renderer::setGlobalTint(float r, float g, float b) {
    m_globalTintR = std::clamp(r, 0.0F, 2.0F);
    m_globalTintG = std::clamp(g, 0.0F, 2.0F);
    m_globalTintB = std::clamp(b, 0.0F, 2.0F);
}

void Renderer::render(const Map& map, const Player& player, const Light& playerLight, const Light& lampLight) {
    if (m_forceCpuPath) {
        renderCpuLighting(map, player, playerLight, lampLight);
        return;
    }

    if (!ensureRenderTargets()) {
        renderCpuLighting(map, player, playerLight, lampLight);
        return;
    }

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, static_cast<double>(m_targetWidth), static_cast<double>(m_targetHeight), 0.0, -1.0, 1.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST);

    glBindFramebuffer(GL_FRAMEBUFFER, m_albedoFbo);
    glViewport(0, 0, m_targetWidth, m_targetHeight);
    glClearColor(0.0F, 0.0F, 0.0F, 1.0F);
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(m_albedoProgram);
    renderSceneAlbedo(map, player);

    glBindFramebuffer(GL_FRAMEBUFFER, m_lightFbo);
    glViewport(0, 0, m_targetWidth, m_targetHeight);
    glClearColor(0.0F, 0.0F, 0.0F, 1.0F);
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(m_lightProgram);
    glUniform2f(glGetUniformLocation(m_lightProgram, "uResolution"), static_cast<float>(m_targetWidth), static_cast<float>(m_targetHeight));
    glUniform2f(glGetUniformLocation(m_lightProgram, "uIsoTile"), kTileW, kTileH);
    glUniform2f(glGetUniformLocation(m_lightProgram, "uIsoOrigin"), kOriginX, kOriginY);
    glUniform1f(glGetUniformLocation(m_lightProgram, "uAmbient"), m_ambient);
    glUniform3f(glGetUniformLocation(m_lightProgram, "uAmbientColor"), 0.68F, 0.74F, 0.84F);
    glUniform4f(glGetUniformLocation(m_lightProgram, "uPlayerLight"), playerLight.x, playerLight.y, playerLight.radius, playerLight.intensity);
    glUniform4f(glGetUniformLocation(m_lightProgram, "uLampLight"), lampLight.x, lampLight.y, lampLight.radius, lampLight.intensity);
    glUniform4f(glGetUniformLocation(m_lightProgram, "uPlayerLightColor"), playerLight.r, playerLight.g, playerLight.b, playerLight.falloffExponent);
    glUniform4f(glGetUniformLocation(m_lightProgram, "uLampLightColor"), lampLight.r, lampLight.g, lampLight.b, lampLight.falloffExponent);
    drawFullscreenQuad();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, m_targetWidth, m_targetHeight);
    glClearColor(0.06F, 0.06F, 0.08F, 1.0F);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(m_compositeProgram);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_albedoTex);
    glUniform1i(glGetUniformLocation(m_compositeProgram, "uAlbedoTex"), 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_lightTex);
    glUniform1i(glGetUniformLocation(m_compositeProgram, "uLightTex"), 1);
    glUniform3f(glGetUniformLocation(m_compositeProgram, "uGlobalTint"), m_globalTintR, m_globalTintG, m_globalTintB);

    drawFullscreenQuad();

    glUseProgram(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE0);
    SDL_GL_SwapWindow(m_window);
}


bool Renderer::loadGlFunctions() {
    auto loadProc = [](auto& proc, const char* name) {
        proc = reinterpret_cast<std::remove_reference_t<decltype(proc)>>(SDL_GL_GetProcAddress(name));
        return proc != nullptr;
    };

    return loadProc(g_gl.activeTexture, "glActiveTexture") &&
        loadProc(g_gl.attachShader, "glAttachShader") &&
        loadProc(g_gl.compileShader, "glCompileShader") &&
        loadProc(g_gl.createProgram, "glCreateProgram") &&
        loadProc(g_gl.createShader, "glCreateShader") &&
        loadProc(g_gl.deleteProgram, "glDeleteProgram") &&
        loadProc(g_gl.deleteShader, "glDeleteShader") &&
        loadProc(g_gl.getProgramiv, "glGetProgramiv") &&
        loadProc(g_gl.getProgramInfoLog, "glGetProgramInfoLog") &&
        loadProc(g_gl.getShaderiv, "glGetShaderiv") &&
        loadProc(g_gl.getShaderInfoLog, "glGetShaderInfoLog") &&
        loadProc(g_gl.getUniformLocation, "glGetUniformLocation") &&
        loadProc(g_gl.linkProgram, "glLinkProgram") &&
        loadProc(g_gl.shaderSource, "glShaderSource") &&
        loadProc(g_gl.useProgram, "glUseProgram") &&
        loadProc(g_gl.uniform1f, "glUniform1f") &&
        loadProc(g_gl.uniform2f, "glUniform2f") &&
        loadProc(g_gl.uniform3f, "glUniform3f") &&
        loadProc(g_gl.uniform4f, "glUniform4f") &&
        loadProc(g_gl.uniform1i, "glUniform1i") &&
        loadProc(g_gl.bindFramebuffer, "glBindFramebuffer") &&
        loadProc(g_gl.deleteFramebuffers, "glDeleteFramebuffers") &&
        loadProc(g_gl.genFramebuffers, "glGenFramebuffers") &&
        loadProc(g_gl.checkFramebufferStatus, "glCheckFramebufferStatus") &&
        loadProc(g_gl.framebufferTexture2D, "glFramebufferTexture2D");
}

bool Renderer::initializeGpuPipeline() {
    GLuint fullscreenVs = compileShader(GL_VERTEX_SHADER, kFullscreenVertexShader, "fullscreen.vert");
    if (fullscreenVs == 0) {
        return false;
    }

    const fs::path albedoPath = resolveResourcePath("assets/shaders/albedo.glsl");
    const fs::path lightPath = resolveResourcePath("assets/shaders/light.glsl");
    const fs::path compositePath = resolveResourcePath("assets/shaders/composite.glsl");

    std::string albedoFragment;
    std::string lightFragment;
    std::string compositeFragment;
    if (!loadShaderSource(albedoPath.string().c_str(), albedoFragment) ||
        !loadShaderSource(lightPath.string().c_str(), lightFragment) ||
        !loadShaderSource(compositePath.string().c_str(), compositeFragment)) {
        std::cerr << "Failed to load shader sources from:\n"
                  << "  " << albedoPath << "\n"
                  << "  " << lightPath << "\n"
                  << "  " << compositePath << "\n";
        glDeleteShader(fullscreenVs);
        return false;
    }

    GLuint albedoFs = compileShader(GL_FRAGMENT_SHADER, albedoFragment.c_str(), "albedo.frag");
    GLuint lightFs = compileShader(GL_FRAGMENT_SHADER, lightFragment.c_str(), "light.frag");
    GLuint compositeFs = compileShader(GL_FRAGMENT_SHADER, compositeFragment.c_str(), "composite.frag");
    if (albedoFs == 0 || lightFs == 0 || compositeFs == 0) {
        glDeleteShader(fullscreenVs);
        if (albedoFs != 0) {
            glDeleteShader(albedoFs);
        }
        if (lightFs != 0) {
            glDeleteShader(lightFs);
        }
        if (compositeFs != 0) {
            glDeleteShader(compositeFs);
        }
        return false;
    }

    m_albedoProgram = linkProgram(fullscreenVs, albedoFs, "albedo");
    m_lightProgram = linkProgram(fullscreenVs, lightFs, "light");
    m_compositeProgram = linkProgram(fullscreenVs, compositeFs, "composite");

    glDeleteShader(fullscreenVs);
    glDeleteShader(albedoFs);
    glDeleteShader(lightFs);
    glDeleteShader(compositeFs);

    if (m_albedoProgram == 0 || m_lightProgram == 0 || m_compositeProgram == 0) {
        destroyGpuPipeline();
        return false;
    }

    return true;
}

void Renderer::destroyGpuPipeline() {
    if (m_albedoTex != 0) {
        glDeleteTextures(1, &m_albedoTex);
        m_albedoTex = 0;
    }
    if (m_lightTex != 0) {
        glDeleteTextures(1, &m_lightTex);
        m_lightTex = 0;
    }

    if (m_albedoFbo != 0) {
        glDeleteFramebuffers(1, &m_albedoFbo);
        m_albedoFbo = 0;
    }
    if (m_lightFbo != 0) {
        glDeleteFramebuffers(1, &m_lightFbo);
        m_lightFbo = 0;
    }

    if (m_albedoProgram != 0) {
        glDeleteProgram(m_albedoProgram);
        m_albedoProgram = 0;
    }
    if (m_lightProgram != 0) {
        glDeleteProgram(m_lightProgram);
        m_lightProgram = 0;
    }
    if (m_compositeProgram != 0) {
        glDeleteProgram(m_compositeProgram);
        m_compositeProgram = 0;
    }
}

bool Renderer::ensureRenderTargets() {
    int width = 0;
    int height = 0;
    SDL_GetWindowSize(m_window, &width, &height);
    if (width <= 0 || height <= 0) {
        return false;
    }

    if (width == m_targetWidth && height == m_targetHeight && m_albedoTex != 0 && m_lightTex != 0) {
        return true;
    }

    if (m_albedoTex != 0) {
        glDeleteTextures(1, &m_albedoTex);
        m_albedoTex = 0;
    }
    if (m_lightTex != 0) {
        glDeleteTextures(1, &m_lightTex);
        m_lightTex = 0;
    }
    if (m_albedoFbo != 0) {
        glDeleteFramebuffers(1, &m_albedoFbo);
        m_albedoFbo = 0;
    }
    if (m_lightFbo != 0) {
        glDeleteFramebuffers(1, &m_lightFbo);
        m_lightFbo = 0;
    }

    m_targetWidth = width;
    m_targetHeight = height;

    glGenTextures(1, &m_albedoTex);
    glBindTexture(GL_TEXTURE_2D, m_albedoTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_targetWidth, m_targetHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glGenTextures(1, &m_lightTex);
    glBindTexture(GL_TEXTURE_2D, m_lightTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_targetWidth, m_targetHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glGenFramebuffers(1, &m_albedoFbo);
    glBindFramebuffer(GL_FRAMEBUFFER, m_albedoFbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_albedoTex, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        return false;
    }

    glGenFramebuffers(1, &m_lightFbo);
    glBindFramebuffer(GL_FRAMEBUFFER, m_lightFbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_lightTex, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        return false;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    return true;
}

bool Renderer::loadShaderSource(const char* path, std::string& outSource) const {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Unable to open shader source: " << path << '\n';
        return false;
    }

    std::stringstream stream;
    stream << file.rdbuf();
    outSource = stream.str();
    return true;
}

GLuint Renderer::compileShader(GLenum shaderType, const char* source, const char* label) const {
    const GLuint shader = glCreateShader(shaderType);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint status = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE) {
        GLint logLength = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
        std::vector<char> log(static_cast<size_t>(std::max(logLength, 1)));
        glGetShaderInfoLog(shader, logLength, nullptr, log.data());
        std::fprintf(stderr, "Failed to compile shader %s: %s\n", label, log.data());
        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

GLuint Renderer::linkProgram(GLuint vertexShader, GLuint fragmentShader, const char* label) const {
    const GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    GLint status = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (status == GL_FALSE) {
        GLint logLength = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
        std::vector<char> log(static_cast<size_t>(std::max(logLength, 1)));
        glGetProgramInfoLog(program, logLength, nullptr, log.data());
        std::fprintf(stderr, "Failed to link %s program: %s\n", label, log.data());
        glDeleteProgram(program);
        return 0;
    }

    return program;
}

void Renderer::renderCpuLighting(const Map& map, const Player& player, const Light& playerLight, const Light& lampLight) const {
    glClearColor(0.06F, 0.06F, 0.08F, 1.0F);
    glClear(GL_COLOR_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    int width = 0;
    int height = 0;
    SDL_GetWindowSize(m_window, &width, &height);
    glOrtho(0.0, static_cast<double>(width), static_cast<double>(height), 0.0, -1.0, 1.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    OcclusionCache playerOcclusion(map.width(), map.height());
    OcclusionCache lampOcclusion(map.width(), map.height());

    glBegin(GL_QUADS);
    for (int y = 0; y < map.height(); ++y) {
        for (int x = 0; x < map.width(); ++x) {
            const bool blocked = map.isBlocked(x, y);
            const float sx = kOriginX + (x - y) * (kTileW * 0.5F);
            const float sy = kOriginY + (x + y) * (kTileH * 0.5F);

            const float playerContribution = directWithOcclusion(map, x, y, playerLight, playerOcclusion);
            const float lampContribution = directWithOcclusion(map, x, y, lampLight, lampOcclusion);
            const float ambient = m_ambient;

            float lightR = 0.68F * ambient + playerLight.r * playerContribution + lampLight.r * lampContribution;
            float lightG = 0.74F * ambient + playerLight.g * playerContribution + lampLight.g * lampContribution;
            float lightB = 0.84F * ambient + playerLight.b * playerContribution + lampLight.b * lampContribution;

            lightR = lightR / (1.0F + lightR);
            lightG = lightG / (1.0F + lightG);
            lightB = lightB / (1.0F + lightB);

            lightR = std::clamp(lightR * m_globalTintR, 0.0F, 1.0F);
            lightG = std::clamp(lightG * m_globalTintG, 0.0F, 1.0F);
            lightB = std::clamp(lightB * m_globalTintB, 0.0F, 1.0F);

            if (blocked) {
                glColor3f(0.42F * lightR, 0.30F * lightG, 0.20F * lightB);
            } else {
                glColor3f(0.67F * lightR, 0.59F * lightG, 0.34F * lightB);
            }

            glVertex2f(sx, sy + kTileH * 0.5F);
            glVertex2f(sx + kTileW * 0.5F, sy);
            glVertex2f(sx + kTileW, sy + kTileH * 0.5F);
            glVertex2f(sx + kTileW * 0.5F, sy + kTileH);
        }
    }
    glEnd();

    const float playerSx = kOriginX + (player.x() - player.y()) * (kTileW * 0.5F) + kTileW * 0.5F;
    const float playerSyBase = kOriginY + (player.x() + player.y()) * (kTileH * 0.5F) + kTileH * 0.5F;

    const float bob = std::sin(player.walkPhase() * 2.0F) * 2.5F * player.moveBlend();
    const float sway = std::sin(player.walkPhase()) * 1.8F * player.moveBlend();
    const float playerSy = playerSyBase - bob;

    glColor3f(0.10F, 0.10F, 0.12F);
    glBegin(GL_QUADS);
    glVertex2f(playerSx - 9.0F, playerSyBase + 2.0F);
    glVertex2f(playerSx + 9.0F, playerSyBase + 2.0F);
    glVertex2f(playerSx + 9.0F, playerSyBase + 6.0F);
    glVertex2f(playerSx - 9.0F, playerSyBase + 6.0F);
    glEnd();

    glColor3f(0.2F, 0.4F, 0.85F);
    glBegin(GL_QUADS);
    glVertex2f(playerSx - 8.0F + sway, playerSy - 20.0F);
    glVertex2f(playerSx + 8.0F + sway, playerSy - 20.0F);
    glVertex2f(playerSx + 8.0F - sway, playerSy);
    glVertex2f(playerSx - 8.0F - sway, playerSy);
    glEnd();

    SDL_GL_SwapWindow(m_window);
}

void Renderer::renderSceneAlbedo(const Map& map, const Player& player) const {
    glBegin(GL_QUADS);
    for (int y = 0; y < map.height(); ++y) {
        for (int x = 0; x < map.width(); ++x) {
            const bool blocked = map.isBlocked(x, y);
            const float sx = kOriginX + (x - y) * (kTileW * 0.5F);
            const float sy = kOriginY + (x + y) * (kTileH * 0.5F);

            if (blocked) {
                glColor3f(0.42F, 0.30F, 0.20F);
            } else {
                glColor3f(0.67F, 0.59F, 0.34F);
            }

            glVertex2f(sx, sy + kTileH * 0.5F);
            glVertex2f(sx + kTileW * 0.5F, sy);
            glVertex2f(sx + kTileW, sy + kTileH * 0.5F);
            glVertex2f(sx + kTileW * 0.5F, sy + kTileH);
        }
    }
    glEnd();

    const float playerSx = kOriginX + (player.x() - player.y()) * (kTileW * 0.5F) + kTileW * 0.5F;
    const float playerSyBase = kOriginY + (player.x() + player.y()) * (kTileH * 0.5F) + kTileH * 0.5F;

    const float bob = std::sin(player.walkPhase() * 2.0F) * 2.5F * player.moveBlend();
    const float sway = std::sin(player.walkPhase()) * 1.8F * player.moveBlend();
    const float playerSy = playerSyBase - bob;

    glColor3f(0.10F, 0.10F, 0.12F);
    glBegin(GL_QUADS);
    glVertex2f(playerSx - 9.0F, playerSyBase + 2.0F);
    glVertex2f(playerSx + 9.0F, playerSyBase + 2.0F);
    glVertex2f(playerSx + 9.0F, playerSyBase + 6.0F);
    glVertex2f(playerSx - 9.0F, playerSyBase + 6.0F);
    glEnd();

    glColor3f(0.2F, 0.4F, 0.85F);
    glBegin(GL_QUADS);
    glVertex2f(playerSx - 8.0F + sway, playerSy - 20.0F);
    glVertex2f(playerSx + 8.0F + sway, playerSy - 20.0F);
    glVertex2f(playerSx + 8.0F - sway, playerSy);
    glVertex2f(playerSx - 8.0F - sway, playerSy);
    glEnd();
}

void Renderer::drawFullscreenQuad() const {
    glColor3f(1.0F, 1.0F, 1.0F);
    glBegin(GL_QUADS);
    glTexCoord2f(0.0F, 0.0F);
    glVertex2f(0.0F, 0.0F);
    glTexCoord2f(1.0F, 0.0F);
    glVertex2f(static_cast<float>(m_targetWidth), 0.0F);
    glTexCoord2f(1.0F, 1.0F);
    glVertex2f(static_cast<float>(m_targetWidth), static_cast<float>(m_targetHeight));
    glTexCoord2f(0.0F, 1.0F);
    glVertex2f(0.0F, static_cast<float>(m_targetHeight));
    glEnd();
}
