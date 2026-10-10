#pragma once

#include "../Shader.h"
#include "../settings/AtmosphereSettings.h"
#include "../settings/CloudSettings.h"
#include <glm/glm.hpp>
#include <cstdint>

namespace planets::render
{
class PostProcessor;
}

namespace planets::render::effects
{

// The volumetric cloud layer: bakes the weather cube map and the 3D noise from the body seed,
// marches the layer at half resolution into an HDR target (rgb light, a transmittance) that the
// atmosphere pass composites, and sets the shared layer uniforms (cloud_layer.glsl) for the
// ground shadow.
class CloudVolume
{
public:
    // Texture units of cloud_layer.glsl.
    static constexpr uint32_t WeatherBinding = 8;
    static constexpr uint32_t ShapeBinding = 9;
    static constexpr uint32_t DetailBinding = 10;

    CloudVolume() = default;
    ~CloudVolume();

    CloudVolume(const CloudVolume&) = delete;
    CloudVolume& operator=(const CloudVolume&) = delete;

    bool Initialize(int width, int height);
    void Resize(int width, int height);

    // Regenerates the noise for this seed; a no-op when the seed is unchanged.
    void Generate(uint32_t seed);

    // Per-frame layer geometry. baseAltitude is the effective cloud base (fraction of radius).
    struct Layer
    {
        glm::vec3 center{0.0f};
        float planetRadius = 1.0f;
        float groundRadius = 1.0f;
        float baseAltitude = 0.025f;
        float time = 0.0f;
    };

    // Uniforms + textures of cloud_layer.glsl on any shader that includes it.
    void SetLayerUniforms(const Shader& shader, const CloudSettings& settings, const Layer& layer) const;

    struct View
    {
        glm::mat4 invView{1.0f};
        glm::mat4 invProjection{1.0f};
        glm::vec3 cameraPos{0.0f};
        float nearPlane = 0.1f;
        float farPlane = 1000.0f;
        glm::vec3 lightDir{0.0f, -1.0f, 0.0f};
        glm::vec3 sunColor{1.0f};
        float sunIntensity = 1.0f;
    };

    // Marches the layer; returns the half-resolution result texture. Leaves its framebuffer bound.
    uint32_t Render(const View& view,
                    const CloudSettings& settings,
                    const Layer& layer,
                    const AtmosphereSettings& atmosphere,
                    uint32_t depthTexture,
                    uint32_t blueNoiseTexture,
                    const PostProcessor& fullscreen);

private:
    void ReleaseTarget();
    void ReleaseNoise();

    Shader _marchShader;
    uint32_t _fbo = 0;
    uint32_t _result = 0;
    int _width = 0;
    int _height = 0;

    uint32_t _weather = 0;
    uint32_t _shape = 0;
    uint32_t _detail = 0;
    uint32_t _seed = 0;
};

} // namespace planets::render::effects
