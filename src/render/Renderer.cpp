#include "Renderer.h"
#include "GpuConstants.h"
#include <GL/gl3w.h>
#include <iostream>

namespace planets::render
{

Renderer::Renderer() = default;

Renderer::~Renderer()
{
    Shutdown();
}

bool Renderer::Initialize(int width, int height)
{
    glEnable(GL_DEPTH_TEST);
    _fbWidth = width;
    _fbHeight = height;

    if (!_sceneFbo.Create(width, height))
    {
        std::cerr << "[Renderer] Failed to create scene framebuffer" << std::endl;
        return false;
    }
    if (!_postProcessor.Initialize())
    {
        std::cerr << "[Renderer] Failed to initialize post-processor" << std::endl;
        return false;
    }
    if (!_passthroughShader.LoadFromFiles("shaders/passthrough.vert", "shaders/passthrough.frag"))
    {
        std::cerr << "[Renderer] Failed to load passthrough shader" << std::endl;
        return false;
    }
    if (!_spaceShader.LoadFromFiles("shaders/space.vert", "shaders/space.frag"))
    {
        std::cerr << "[Renderer] Failed to load space shader" << std::endl;
        return false;
    }
    return true;
}

void Renderer::Shutdown() {}

void Renderer::ResizeFramebuffer(int width, int height)
{
    _fbWidth = width;
    _fbHeight = height;
    _sceneFbo.Create(width, height);
}

bool Renderer::SetActiveBody(PgBody body,
                             const BodyRuntime& runtime,
                             const lod::QuadTreeConfig& quadTreeConfig,
                             float seaLevel,
                             uint32_t seed)
{
    if (!_planetShader.LoadFromFiles(runtime.GetVertexShaderPath(), runtime.GetFragmentShaderPath()))
    {
        std::cerr << "[Renderer] Failed to load planet shader for " << runtime.GetTypeName() << std::endl;
        return false;
    }

    _palette = runtime.LoadBiomePalette();
    if (_palette.IsValid())
        _palette.UploadToGpu(_paletteBuffer);

    _scheduler.Initialize(body, seed);

    _quadTree.Initialize(quadTreeConfig, _scheduler, seed);
    _oceanRenderer.Initialize(quadTreeConfig.planetRadius, seaLevel, OceanSubdivisions);
    _atmosphereRenderer.Initialize();
    return true;
}

void Renderer::DrawScene(const RenderContext& ctx)
{
    const glm::mat4 invView       = glm::inverse(ctx.view);
    const glm::mat4 invProjection = glm::inverse(ctx.projection);
    const glm::mat4 viewProjection = ctx.projection * ctx.view;

    _sceneFbo.Bind();
    // ImGui leaves depth-test disabled; the planet must write depth so the
    // atmosphere pass can tell surface from sky. Re-enable it each frame.
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glClearColor(0.1f, 0.1f, 0.2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    DrawSpace(ctx, invView, invProjection);
    DrawPlanet(ctx, viewProjection);

    if (ctx.body && ctx.seaLevel > 0.0f && ctx.ocean && ctx.scene)
    {
        _oceanRenderer.Render(ctx.view, ctx.projection, ctx.cameraPos,
                              ctx.scene->lightDir, ctx.time, *ctx.ocean);
    }

    _sceneFbo.Unbind();
    glViewport(0, 0, _fbWidth, _fbHeight);

    glClearColor(0.1f, 0.1f, 0.2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    DrawAtmosphereOrPassthrough(ctx, invView, invProjection);
}

void Renderer::DrawSpace(const RenderContext& ctx, const glm::mat4& invView, const glm::mat4& invProjection)
{
    if (!ctx.scene)
        return;

    glDepthMask(GL_FALSE);
    _spaceShader.Use();
    _spaceShader.SetMat4("uInvProjection", invProjection);
    _spaceShader.SetMat4("uInvView", invView);
    _spaceShader.SetVec3("uLightDir", ctx.scene->lightDir);
    _spaceShader.SetFloat("uSunSize", ctx.scene->sunSize);
    _spaceShader.SetVec3("uSunColor", ctx.scene->sunColor);
    _spaceShader.SetFloat("uStarDensity", ctx.scene->starDensity);
    _postProcessor.RenderQuad();
    glDepthMask(GL_TRUE);
}

void Renderer::DrawPlanet(const RenderContext& ctx, const glm::mat4& viewProjection)
{
    // Generate/complete patches first: per-patch generation runs the library's
    // compute shaders, which leave their program bound. Bind the planet shader
    // afterwards so that compute program can't clobber the active raster program —
    // otherwise the patch draw runs the compute program and rasterizes nothing.
    _scheduler.ProcessFrame(SchedulerPatchesPerFrame, SchedulerPatchesPerFrame);
    _quadTree.ApplyCompletedPatches();
    _quadTree.Update(ctx.cameraPos, viewProjection);

    _planetShader.Use();
    _planetShader.SetMat4("uModel", glm::mat4(1.0f));
    _planetShader.SetMat4("uView", ctx.view);
    _planetShader.SetMat4("uProjection", ctx.projection);
    _planetShader.SetVec3("uCameraPos", ctx.cameraPos);
    _planetShader.SetFloat("uSeaLevel", ctx.seaLevel);

    if (ctx.scene)
    {
        _planetShader.SetVec3("uLightDir", ctx.scene->lightDir);
        _planetShader.SetFloat("uSunIntensity", ctx.scene->lighting.sunIntensity);
        _planetShader.SetFloat("uAmbientLight", ctx.scene->lighting.ambientLight);
        _planetShader.SetFloat("uSpecularStrength", ctx.scene->lighting.specularStrength);
        _planetShader.SetFloat("uSpecularPower", ctx.scene->lighting.specularPower);
        _planetShader.SetFloat("uDetailFadeStart", ctx.scene->lighting.detailFadeStart);
        _planetShader.SetFloat("uHazeStrength", ctx.scene->lighting.hazeStrength);
        _planetShader.SetVec3("uHazeColor", ctx.scene->lighting.hazeColor);
    }

    if (ctx.body)
        ctx.body->SetRenderUniforms(_planetShader);

    _planetShader.SetFloat("uPlanetScale", ctx.planetRadius);
    _planetShader.SetFloat("uFresnelStrengthNear", FresnelStrengthNear);
    _planetShader.SetFloat("uFresnelStrengthFar", FresnelStrengthFar);
    _planetShader.SetFloat("uFresnelPow", FresnelPower);

    if (ctx.biome)
        _planetShader.SetFloat("uAOStrength", ctx.biome->aoStrength);

    if (_paletteBuffer.IsValid())
    {
        _paletteBuffer.Bind(BiomePaletteBindingPoint);
        _planetShader.SetInt("uPaletteSize", _palette.GetEntryCount());
    }

    _quadTree.Render(_planetShader);
}

void Renderer::DrawAtmosphereOrPassthrough(const RenderContext& ctx, const glm::mat4& invView,
                                           const glm::mat4& invProjection)
{
    if (ctx.atmosphere && ctx.atmosphere->enabled && ctx.scene)
    {
        const float oceanRadius = ctx.planetRadius * ctx.seaLevel;
        _atmosphereRenderer.Render(ctx.view, ctx.projection, invView, invProjection,
                                   ctx.cameraPos, ctx.scene->lightDir,
                                   glm::vec3(0.0f), ctx.planetRadius, oceanRadius,
                                   ctx.nearPlane, ctx.farPlane, *ctx.atmosphere,
                                   _sceneFbo.GetColorTexture(), _sceneFbo.GetDepthTexture());
    }
    else
    {
        _passthroughShader.Use();
        _passthroughShader.SetInt("uSceneTexture", 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, _sceneFbo.GetColorTexture());
        _postProcessor.RenderQuad();
    }
}

} // namespace planets::render
