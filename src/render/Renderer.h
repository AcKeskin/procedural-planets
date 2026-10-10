#pragma once

#include "Shader.h"
#include "Framebuffer.h"
#include "PostProcessor.h"
#include "GenerationScheduler.h"
#include "BodyRuntime.h"
#include "BiomePalette.h"
#include "GpuBuffer.h"
#include "GpuTimer.h"
#include "lod/PlanetQuadTree.h"
#include "effects/OceanRenderer.h"
#include "effects/AtmosphereRenderer.h"
#include "effects/CloudRenderer.h"
#include "effects/BloomPass.h"
#include "settings/CloudSettings.h"
#include "settings/SceneSettings.h"
#include "settings/OceanSettings.h"
#include "settings/AtmosphereSettings.h"
#include <planetgen/planetgen.h>
#include <glm/glm.hpp>

namespace planets::render
{

// Per-frame state the scene draw reads but does not own. The shell assembles it from
// the camera and its settings; the renderer never reaches back into shell state.
struct RenderContext
{
    glm::mat4 view{1.0f};
    glm::mat4 projection{1.0f};
    glm::vec3 cameraPos{0.0f};
    float nearPlane = 0.1f;
    float farPlane = 1000.0f;
    float time = 0.0f;

    const SceneSettings* scene = nullptr;
    const effects::OceanSettings* ocean = nullptr;
    const effects::AtmosphereSettings* atmosphere = nullptr;
    const effects::CloudSettings* clouds = nullptr;

    const BodyRuntime* body = nullptr; // null when no body loaded; carries shading/AO via Config()
    float planetRadius = 1.0f;
    float seaLevel = 0.0f; // radial multiplier: the one sea level terrain, ocean and atmosphere share

    float SeaLevelRadius() const { return planetRadius * seaLevel; }
};

// GPU time of the scene passes, in milliseconds (smoothed, a few frames behind).
struct GpuTimings
{
    float terrainMs = 0.0f;
    float oceanMs = 0.0f;
    float effectsMs = 0.0f; // clouds + atmosphere/post
    float TotalMs() const { return terrainMs + oceanMs + effectsMs; }
};

// Owns the planet draw + LOD pipeline: quad-tree, async patch scheduler, surface/
// space/post shaders, scene framebuffer, and the ocean/atmosphere effect passes.
// The shell drives it via SetActiveBody + DrawScene; it never touches window/input.
class Renderer
{
public:
    Renderer();
    ~Renderer();

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    bool Initialize(int width, int height);
    void Shutdown();

    void ResizeFramebuffer(int width, int height);

    // Build the LOD pipeline around a library body: load its shaders, build the
    // quad-tree, init effects, upload the palette.
    bool
    SetActiveBody(PgBody body, const BodyRuntime& runtime, const lod::QuadTreeConfig& quadTreeConfig, uint32_t seed);

    // Composite one frame into the scene framebuffer then to the default target.
    void DrawScene(const RenderContext& ctx);

    // Capture/cinematic reads the composited scene from here.
    Framebuffer& SceneFramebuffer() { return _sceneFbo; }

    // The size the scene is actually drawn + presented at. A capture must read pixels at
    // THIS size (not the window size, which can change between a draw and the readback in
    // live mode), or glReadPixels reads a region the backbuffer was never filled to.
    int FramebufferWidth() const { return _fbWidth; }
    int FramebufferHeight() const { return _fbHeight; }

    GpuTimings Timings() const
    {
        return {_terrainTimer.Milliseconds(), _oceanTimer.Milliseconds(), _effectsTimer.Milliseconds()};
    }

    GenerationScheduler& Scheduler() { return _scheduler; }
    lod::PlanetQuadTree& QuadTree() { return _quadTree; }

private:
    void DrawSpace(const RenderContext& ctx, const glm::mat4& invView, const glm::mat4& invProjection);
    void DrawPlanet(const RenderContext& ctx, const glm::mat4& viewProjection);
    void
    DrawAtmosphereOrPassthrough(const RenderContext& ctx, const glm::mat4& invView, const glm::mat4& invProjection);

    Framebuffer _sceneFbo;
    PostProcessor _postProcessor;

    Shader _planetShader;
    Shader _spaceShader;
    Shader _passthroughShader;

    GenerationScheduler _scheduler;
    lod::PlanetQuadTree _quadTree;
    effects::OceanRenderer _oceanRenderer;
    effects::AtmosphereRenderer _atmosphereRenderer;
    effects::CloudRenderer _cloudRenderer;
    effects::BloomPass _bloom;

    BiomePalette _palette;
    GpuBuffer<BiomeEntry> _paletteBuffer;

    int _fbWidth = 0;
    int _fbHeight = 0;

    // Back-to-back (a GL_TIME_ELAPSED query cannot nest), so their sum is the scene time.
    GpuTimer _terrainTimer;
    GpuTimer _oceanTimer;
    GpuTimer _effectsTimer;
};

} // namespace planets::render
