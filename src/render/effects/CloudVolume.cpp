#include "CloudVolume.h"
#include "AtmosphereRenderer.h"
#include "../PostProcessor.h"
#include <GL/gl3w.h>
#include <materials/CloudNoise.h>
#include <algorithm>
#include <cmath>
#include <chrono>
#include <iostream>

namespace planets::render::effects
{

namespace
{
constexpr int ShapeSize = 64;
constexpr int DetailSize = 32;
constexpr int WeatherFaceSize = 512;
// Shape tiles per planet radius at scale 1: a cloud cell is about a hundredth of the radius.
constexpr float ShapeTilesPerRadius = 25.0f;
// Weather drift in radians per second at speed 1.
constexpr float DriftRate = 0.01f;

uint32_t CreateVolume(const std::vector<uint8_t>& texels, int size)
{
    uint32_t texture = 0;
    glCreateTextures(GL_TEXTURE_3D, 1, &texture);
    const int levels = 1 + static_cast<int>(std::log2(static_cast<float>(size)));
    glTextureStorage3D(texture, levels, GL_R8, size, size, size);
    glTextureSubImage3D(texture, 0, 0, 0, 0, size, size, size, GL_RED, GL_UNSIGNED_BYTE, texels.data());
    glGenerateTextureMipmap(texture);
    glTextureParameteri(texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTextureParameteri(texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(texture, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTextureParameteri(texture, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTextureParameteri(texture, GL_TEXTURE_WRAP_R, GL_REPEAT);
    return texture;
}

uint32_t CreateWeather(const std::vector<uint8_t>& texels, int faceSize)
{
    uint32_t texture = 0;
    glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &texture);
    const int levels = 1 + static_cast<int>(std::log2(static_cast<float>(faceSize)));
    glTextureStorage2D(texture, levels, GL_RG8, faceSize, faceSize);
    const size_t faceBytes = static_cast<size_t>(faceSize) * faceSize * 2;
    for (int face = 0; face < 6; ++face)
    {
        glTextureSubImage3D(
            texture, 0, 0, 0, face, faceSize, faceSize, 1, GL_RG, GL_UNSIGNED_BYTE, texels.data() + face * faceBytes);
    }
    glGenerateTextureMipmap(texture);
    glTextureParameteri(texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTextureParameteri(texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(texture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(texture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    return texture;
}
} // namespace

CloudVolume::~CloudVolume()
{
    ReleaseTarget();
    ReleaseNoise();
}

bool CloudVolume::Initialize(int width, int height)
{
    if (!_marchShader.LoadFromFiles("shaders/passthrough.vert", "shaders/clouds_march.frag"))
    {
        std::cerr << "[CloudVolume] Failed to load the cloud march shader" << std::endl;
        return false;
    }
    // Cube map lookups blend across face edges, so the weather has no seams.
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    Resize(width, height);
    return _fbo != 0;
}

void CloudVolume::Resize(int width, int height)
{
    ReleaseTarget();
    _width = std::max(1, width / 2);
    _height = std::max(1, height / 2);
    glCreateTextures(GL_TEXTURE_2D, 1, &_result);
    glTextureStorage2D(_result, 1, GL_RGBA16F, _width, _height);
    glTextureParameteri(_result, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(_result, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(_result, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(_result, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glCreateFramebuffers(1, &_fbo);
    glNamedFramebufferTexture(_fbo, GL_COLOR_ATTACHMENT0, _result, 0);
    if (glCheckNamedFramebufferStatus(_fbo, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        std::cerr << "[CloudVolume] Framebuffer incomplete" << std::endl;
        ReleaseTarget();
    }
}

void CloudVolume::Generate(uint32_t seed)
{
    if (_weather != 0 && seed == _seed)
        return;
    ReleaseNoise();
    const auto start = std::chrono::steady_clock::now();
    _shape = CreateVolume(planetgen::GenerateCloudShapeNoise(seed, ShapeSize), ShapeSize);
    _detail = CreateVolume(planetgen::GenerateCloudDetailNoise(seed, DetailSize), DetailSize);
    _weather = CreateWeather(planetgen::GenerateCloudWeather(seed, WeatherFaceSize), WeatherFaceSize);
    _seed = seed;
    const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
    std::cout << "[CloudVolume] Generated cloud noise in " << ms << " ms" << std::endl;
}

void CloudVolume::SetLayerUniforms(const Shader& shader, const CloudSettings& settings, const Layer& layer) const
{
    const float base = layer.planetRadius * (1.0f + layer.baseAltitude);
    shader.SetVec3("uCloudCenter", layer.center);
    shader.SetFloat("uCloudPlanetRadius", layer.planetRadius);
    shader.SetFloat("uCloudBaseRadius", base);
    shader.SetFloat("uCloudTopRadius", base + layer.planetRadius * std::max(settings.thickness, 0.001f));
    shader.SetFloat("uCloudCoverage", settings.coverage);
    shader.SetFloat("uCloudDensity", settings.density);
    shader.SetFloat("uCloudShapeScale", ShapeTilesPerRadius * settings.scale);
    shader.SetFloat("uCloudDrift", layer.time * settings.speed * DriftRate);
    glBindTextureUnit(WeatherBinding, _weather);
    glBindTextureUnit(ShapeBinding, _shape);
    glBindTextureUnit(DetailBinding, _detail);
}

uint32_t CloudVolume::Render(const View& view,
                             const CloudSettings& settings,
                             const Layer& layer,
                             const AtmosphereSettings& atmosphere,
                             uint32_t depthTexture,
                             uint32_t blueNoiseTexture,
                             const PostProcessor& fullscreen)
{
    if (_fbo == 0 || _weather == 0)
        return 0;

    glBindFramebuffer(GL_FRAMEBUFFER, _fbo);
    glViewport(0, 0, _width, _height);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    _marchShader.Use();
    SetLayerUniforms(_marchShader, settings, layer);
    SetSkyUniforms(_marchShader, atmosphere, layer.center, layer.planetRadius, layer.groundRadius);
    _marchShader.SetMat4("uInvView", view.invView);
    _marchShader.SetMat4("uInvProjection", view.invProjection);
    _marchShader.SetVec3("uCameraPos", view.cameraPos);
    _marchShader.SetFloat("uNearPlane", view.nearPlane);
    _marchShader.SetFloat("uFarPlane", view.farPlane);
    _marchShader.SetVec3("uLightDir", glm::normalize(view.lightDir));
    _marchShader.SetVec3("uSunColor", view.sunColor);
    _marchShader.SetFloat("uSunIntensity", view.sunIntensity);
    _marchShader.SetInt("uViewSteps", std::max(settings.viewSteps, 4));
    // Vertical field of view from the projection: tan(fov/2) = 1 / proj[1][1].
    const float tanHalf = 1.0f / glm::inverse(view.invProjection)[1][1];
    _marchShader.SetFloat("uPixelAngle", 2.0f * tanHalf / static_cast<float>(_height));
    // Fixed: with no temporal accumulation a moving jitter only adds flicker.
    _marchShader.SetFloat("uFrameJitter", 0.0f);
    _marchShader.SetInt("uDepthTexture", 0);
    _marchShader.SetInt("uBlueNoise", 1);
    glBindTextureUnit(0, depthTexture);
    glBindTextureUnit(1, blueNoiseTexture);

    fullscreen.RenderQuad();
    glEnable(GL_DEPTH_TEST);
    return _result;
}

void CloudVolume::ReleaseTarget()
{
    if (_fbo)
        glDeleteFramebuffers(1, &_fbo);
    if (_result)
        glDeleteTextures(1, &_result);
    _fbo = 0;
    _result = 0;
}

void CloudVolume::ReleaseNoise()
{
    for (uint32_t* t : {&_weather, &_shape, &_detail})
    {
        if (*t)
            glDeleteTextures(1, t);
        *t = 0;
    }
}

} // namespace planets::render::effects
