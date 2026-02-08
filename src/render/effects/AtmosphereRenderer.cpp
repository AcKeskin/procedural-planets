#include "AtmosphereRenderer.h"
#include <GL/gl3w.h>
#include <glm/glm.hpp>
#include <cmath>
#include <iostream>
#include <vector>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

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

    if (!LoadBlueNoiseTexture())
    {
        std::cerr << "[AtmosphereRenderer] Failed to load blue noise texture" << std::endl;
        return false;
    }

    _initialized = true;
    std::cout << "[AtmosphereRenderer] Initialized" << std::endl;
    return true;
}

bool AtmosphereRenderer::LoadBlueNoiseTexture()
{
    constexpr int size = 64;
    std::vector<unsigned char> data(size * size);

    int width, height, channels;
    unsigned char* fileData = stbi_load("textures/blue_noise_64.png", &width, &height, &channels, 1);

    if (fileData)
    {
        glGenTextures(1, &_blueNoiseTexture);
        glBindTexture(GL_TEXTURE_2D, _blueNoiseTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, fileData);
        stbi_image_free(fileData);
        std::cout << "[AtmosphereRenderer] Loaded blue noise texture (" << width << "x" << height << ")" << std::endl;
    }
    else
    {
        // Interleaved gradient noise as fallback
        std::cout << "[AtmosphereRenderer] Generating procedural blue noise texture" << std::endl;
        for (int y = 0; y < size; ++y)
        {
            for (int x = 0; x < size; ++x)
            {
                float noise = std::fmod(52.9829189f * std::fmod(0.06711056f * x + 0.00583715f * y, 1.0f), 1.0f);
                data[y * size + x] = static_cast<unsigned char>(noise * 255.0f);
            }
        }

        glGenTextures(1, &_blueNoiseTexture);
        glBindTexture(GL_TEXTURE_2D, _blueNoiseTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, size, size, 0, GL_RED, GL_UNSIGNED_BYTE, data.data());
    }

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

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
    if (_blueNoiseTexture != 0)
    {
        glDeleteTextures(1, &_blueNoiseTexture);
        _blueNoiseTexture = 0;
    }
    _initialized = false;
}

void AtmosphereRenderer::CreateFullscreenQuad()
{
    // Fullscreen triangle (more efficient than quad, covers screen in one primitive)
    float vertices[] = {
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
        return;

    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    _atmosphereShader.Use();

    _atmosphereShader.SetMat4("uInvView", invView);
    _atmosphereShader.SetMat4("uInvProjection", invProjection);
    _atmosphereShader.SetVec3("uCameraPos", cameraPos);
    _atmosphereShader.SetVec3("uLightDir", glm::normalize(lightDir));

    _atmosphereShader.SetVec3("uPlanetCenter", planetCenter);
    _atmosphereShader.SetFloat("uPlanetRadius", planetRadius);
    _atmosphereShader.SetFloat("uAtmosphereRadius", planetRadius * (1.0f + settings.atmosphereScale));
    _atmosphereShader.SetFloat("uOceanRadius", oceanRadius);

    _atmosphereShader.SetFloat("uNearPlane", nearPlane);
    _atmosphereShader.SetFloat("uFarPlane", farPlane);

    // Rayleigh coefficients: inverse 4th power of wavelength, normalized by planet radius
    float scatterX = std::pow(400.0f / settings.wavelengths.x, 4.0f);
    float scatterY = std::pow(400.0f / settings.wavelengths.y, 4.0f);
    float scatterZ = std::pow(400.0f / settings.wavelengths.z, 4.0f);
    glm::vec3 scatterCoeffs = glm::vec3(scatterX, scatterY, scatterZ) * settings.scatteringStrength / planetRadius;
    _atmosphereShader.SetVec3("uScatteringCoefficients", scatterCoeffs);

    _atmosphereShader.SetInt("uNumInScatteringPoints", settings.numInScatteringPoints);
    _atmosphereShader.SetInt("uNumOpticalDepthPoints", settings.numOpticalDepthPoints);
    _atmosphereShader.SetFloat("uDensityFalloff", settings.densityFalloff);
    _atmosphereShader.SetFloat("uIntensity", settings.intensity);
    _atmosphereShader.SetFloat("uMieCoefficient", settings.mieCoefficient);
    _atmosphereShader.SetFloat("uMieDensityFalloff", settings.mieDensityFalloff);

    _atmosphereShader.SetFloat("uDitherStrength", settings.ditherStrength);
    _atmosphereShader.SetFloat("uDitherScale", settings.ditherScale);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, sceneTexture);
    _atmosphereShader.SetInt("uSceneTexture", 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, depthTexture);
    _atmosphereShader.SetInt("uDepthTexture", 1);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, _blueNoiseTexture);
    _atmosphereShader.SetInt("uBlueNoise", 2);

    glBindVertexArray(_quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);

    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
}

} // namespace planets::render::effects
