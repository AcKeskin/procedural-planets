#pragma once

#include "../Mesh.h"
#include "../Shader.h"
#include <glm/glm.hpp>
#include "../settings/AtmosphereSettings.h"

namespace planets::render::effects
{

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
    void Render(const glm::mat4& view,
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
