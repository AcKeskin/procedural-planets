#include "AtmosphereRenderer.h"
#include <GL/gl3w.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <iostream>

namespace planets::render::effects {

AtmosphereRenderer::AtmosphereRenderer() = default;

AtmosphereRenderer::~AtmosphereRenderer()
{
    Shutdown();
}

bool AtmosphereRenderer::Initialize()
{
    if (!_atmosphereShader.LoadFromFiles("shaders/atmosphere.vert", "shaders/atmosphere.frag"))
    {
        std::cerr << "[AtmosphereRenderer] Failed to load atmosphere shaders" << std::endl;
        return false;
    }

    CreateFullscreenQuad();

    _initialized = true;
    std::cout << "[AtmosphereRenderer] Initialized with raymarching scattering" << std::endl;
    return true;
}

void AtmosphereRenderer::Shutdown()
{
    if (_quadVAO != 0)
    {
        glDeleteVertexArrays(1, &_quadVAO);
        _quadVAO = 0;
    }
    if (_quadVBO != 0)
    {
        glDeleteBuffers(1, &_quadVBO);
        _quadVBO = 0;
    }
    _initialized = false;
}

void AtmosphereRenderer::CreateFullscreenQuad()
{
    // Fullscreen triangle (more efficient than quad)
    float vertices[] = {
        // positions   // uvs
        -1.0f, -1.0f,  0.0f, 0.0f,
         3.0f, -1.0f,  2.0f, 0.0f,
        -1.0f,  3.0f,  0.0f, 2.0f,
    };

    glGenVertexArrays(1, &_quadVAO);
    glGenBuffers(1, &_quadVBO);

    glBindVertexArray(_quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, _quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

    glBindVertexArray(0);
}

void AtmosphereRenderer::Render(
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
    unsigned int depthTexture)
{
    if (!_initialized || !settings.enabled)
    {
        return;
    }

    // No depth test/write for post-process
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    _atmosphereShader.Use();

    // Camera matrices for ray reconstruction
    _atmosphereShader.SetMat4("uInvView", invView);
    _atmosphereShader.SetMat4("uInvProjection", invProjection);
    _atmosphereShader.SetVec3("uCameraPos", cameraPos);

    // Light direction
    _atmosphereShader.SetVec3("uLightDir", glm::normalize(lightDir));

    // Planet parameters
    _atmosphereShader.SetVec3("uPlanetCenter", planetCenter);
    _atmosphereShader.SetFloat("uPlanetRadius", planetRadius);
    _atmosphereShader.SetFloat("uAtmosphereRadius", planetRadius * (1.0f + settings.atmosphereScale));
    _atmosphereShader.SetFloat("uOceanRadius", oceanRadius);

    // Camera near/far for depth reconstruction
    _atmosphereShader.SetFloat("uNearPlane", nearPlane);
    _atmosphereShader.SetFloat("uFarPlane", farPlane);

    // Rayleigh scattering coefficients
    // Scattering is inversely proportional to wavelength^4
    float scatterX = std::pow(400.0f / settings.wavelengths.x, 4.0f);
    float scatterY = std::pow(400.0f / settings.wavelengths.y, 4.0f);
    float scatterZ = std::pow(400.0f / settings.wavelengths.z, 4.0f);
    glm::vec3 scatterCoeffs = glm::vec3(scatterX, scatterY, scatterZ) * settings.scatteringStrength;
    _atmosphereShader.SetVec3("uScatteringCoefficients", scatterCoeffs);

    // Quality and density
    _atmosphereShader.SetInt("uNumInScatteringPoints", settings.numInScatteringPoints);
    _atmosphereShader.SetInt("uNumOpticalDepthPoints", settings.numOpticalDepthPoints);
    _atmosphereShader.SetFloat("uDensityFalloff", settings.densityFalloff);
    _atmosphereShader.SetFloat("uIntensity", settings.intensity);

    // Bind textures
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, sceneTexture);
    _atmosphereShader.SetInt("uSceneTexture", 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, depthTexture);
    _atmosphereShader.SetInt("uDepthTexture", 1);

    // Draw fullscreen triangle
    glBindVertexArray(_quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);

    // Restore state
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
}

} // namespace planets::render::effects
