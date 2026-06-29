#pragma once

#include "Shader.h"
#include "Framebuffer.h"
#include "PostProcessor.h"
#include "GenerationScheduler.h"
#include "BodyRuntime.h"
#include "BiomePalette.h"
#include "GpuBuffer.h"
#include "lod/PlanetQuadTree.h"
#include "effects/OceanRenderer.h"
#include "effects/AtmosphereRenderer.h"
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

    const BodyRuntime* body = nullptr; // null when no body loaded; carries shading/AO via Config()
    float planetRadius = 1.0f;
    float seaLevel = 0.0f;
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
    bool SetActiveBody(PgBody body,
                       const BodyRuntime& runtime,
                       const lod::QuadTreeConfig& quadTreeConfig,
                       float seaLevel,
                       uint32_t seed);

    // Composite one frame into the scene framebuffer then to the default target.
    void DrawScene(const RenderContext& ctx);

    // Capture/cinematic reads the composited scene from here.
    Framebuffer& SceneFramebuffer() { return _sceneFbo; }

    GenerationScheduler& Scheduler() { return _scheduler; }
    lod::PlanetQuadTree& QuadTree()  { return _quadTree; }

private:
    void DrawSpace(const RenderContext& ctx, const glm::mat4& invView, const glm::mat4& invProjection);
    void DrawPlanet(const RenderContext& ctx, const glm::mat4& viewProjection);
    void DrawAtmosphereOrPassthrough(const RenderContext& ctx, const glm::mat4& invView,
                                     const glm::mat4& invProjection);

    Framebuffer   _sceneFbo;
    PostProcessor _postProcessor;

    Shader _planetShader;
    Shader _spaceShader;
    Shader _passthroughShader;

    GenerationScheduler _scheduler;
    lod::PlanetQuadTree _quadTree;
    effects::OceanRenderer _oceanRenderer;
    effects::AtmosphereRenderer _atmosphereRenderer;

    BiomePalette _palette;
    GpuBuffer<BiomeEntry> _paletteBuffer;

    int _fbWidth = 0;
    int _fbHeight = 0;
};

} // namespace planets::render
