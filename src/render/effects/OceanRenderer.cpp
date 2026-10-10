#include "OceanRenderer.h"
#include "../lod/PlanetQuadTree.h"
#include <GL/gl3w.h>
#include <iostream>

namespace planets::render::effects
{

bool OceanRenderer::Initialize()
{
    if (!_oceanShader.LoadFromFiles("shaders/ocean.vert", "shaders/ocean.frag"))
    {
        std::cerr << "[OceanRenderer] Failed to load ocean shaders" << std::endl;
        _initialized = false;
        return false;
    }
    _initialized = true;
    return true;
}

void OceanRenderer::Render(const OceanFrame& frame, const OceanSettings& settings, const lod::PlanetQuadTree& patches)
{
    if (!_initialized || !settings.enabled)
        return;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    _oceanShader.Use();
    _oceanShader.SetMat4("uView", frame.view);
    _oceanShader.SetMat4("uProjection", frame.projection);
    _oceanShader.SetFloat("uSeaLevel", frame.seaLevel);
    _oceanShader.SetFloat("uHeightScale", frame.heightScale);
    _oceanShader.SetVec3("uCameraPos", frame.cameraPos);
    _oceanShader.SetVec3("uLightDir", frame.lightDir);
    _oceanShader.SetVec3("uSunColor", frame.sunColor);
    _oceanShader.SetFloat("uSunIntensity", frame.sunIntensity);
    _oceanShader.SetFloat("uAmbientLight", frame.ambientLight);
    _oceanShader.SetFloat("uTime", frame.time);
    _oceanShader.SetVec3("uDeepColor", settings.deepColor);
    _oceanShader.SetVec3("uShallowColor", settings.shallowColor);
    _oceanShader.SetFloat("uFresnelPower", settings.fresnelPower);
    _oceanShader.SetFloat("uWaveStrength", settings.waveStrength);
    _oceanShader.SetFloat("uWaveScale", settings.waveScale);
    _oceanShader.SetFloat("uWaveSpeed", settings.waveSpeed);

    // Same visible patches, same per-patch uniforms (resolution, radius, morph) as the planet pass.
    patches.Render(_oceanShader);

    glDisable(GL_BLEND);
}

} // namespace planets::render::effects
