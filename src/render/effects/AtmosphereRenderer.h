#pragma once

#include "../Mesh.h"
#include "../Shader.h"
#include <glm/glm.hpp>

namespace planets::render::effects {

// Physically-based atmosphere settings (defaults from Unity Earth "Humble Abode")
struct AtmosphereSettings
{
    bool enabled = true;

    // Atmosphere geometry
    float atmosphereScale = 0.322f;     // Height as fraction of planet radius

    // Rayleigh scattering wavelengths (nm) - determines sky color
    glm::vec3 wavelengths = glm::vec3(700.0f, 530.0f, 460.0f);  // R, G, B
    float scatteringStrength = 21.23f;  // Overall scattering intensity

    // Density parameters
    float densityFalloff = 4.3f;        // How quickly density decreases with altitude

    // Rendering quality
    int numInScatteringPoints = 10;     // Ray samples for in-scattering
    int numOpticalDepthPoints = 10;     // Ray samples for optical depth (Unity uses 100, but costly)

    // Visual parameters
    float intensity = 1.0f;             // Overall brightness multiplier

    // Dithering parameters (reduces banding)
    float ditherStrength = 0.8f;
    float ditherScale = 4.0f;
};

// Post-process atmosphere renderer using raymarching
class AtmosphereRenderer
{
public:
    AtmosphereRenderer();
    ~AtmosphereRenderer();

    AtmosphereRenderer(const AtmosphereRenderer&) = delete;
    AtmosphereRenderer& operator=(const AtmosphereRenderer&) = delete;

    bool Initialize();
    void Shutdown();

    // Render atmosphere as post-process effect
    // Requires depth buffer to be bound for scene depth sampling
    void Render(
        const glm::mat4& view,
        const glm::mat4& projection,
        const glm::mat4& invView,
        const glm::mat4& invProjection,
        const glm::vec3& cameraPos,
        const glm::vec3& lightDir,
        const glm::vec3& planetCenter,
        float planetRadius,
        float oceanRadius,
        float nearPlane,
        float farPlane,
        const AtmosphereSettings& settings,
        unsigned int sceneTexture,
        unsigned int depthTexture);

    bool IsReady() const { return _initialized; }

private:
    void CreateFullscreenQuad();
    bool LoadBlueNoiseTexture();

    Shader _atmosphereShader;
    unsigned int _quadVAO = 0;
    unsigned int _quadVBO = 0;
    unsigned int _blueNoiseTexture = 0;
    bool _initialized = false;
};

} // namespace planets::render::effects
