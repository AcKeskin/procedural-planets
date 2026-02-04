#pragma once

#include "ComputeShader.h"
#include "GpuBuffer.h"
#include <glm/glm.hpp>
#include <vector>
#include <cstdint>

namespace planets::render {

// Noise layer configuration for compute shader
struct ComputeNoiseParams
{
    glm::vec3 offset = glm::vec3(0.0f);
    int numLayers = 4;
    float scale = 1.0f;
    float persistence = 0.5f;
    float lacunarity = 2.0f;
    float multiplier = 1.0f;
    float power = 1.0f;        // For ridge noise
    float gain = 1.0f;         // For ridge noise
    float smoothOffset = 1.0f; // For smoothed ridge noise
};

// Earth terrain settings (balanced defaults for Earth-like appearance)
struct EarthTerrainSettings
{
    // Continental noise (large-scale landmass shapes)
    int continentOctaves = 6;
    float continentScale = 0.8f;       // Lower = larger continents
    float continentPersistence = 0.5f;
    float continentLacunarity = 2.0f;
    float continentStrength = 2.0f;    // Moderate elevation variation
    float continentBaseLevel = -0.35f; // Less negative = more land (Earth ~30% land)

    // Mountain ridge noise (sharp peaks)
    int mountainOctaves = 5;
    float mountainScale = 1.5f;
    float mountainPersistence = 0.5f;
    float mountainLacunarity = 4.0f;
    float mountainStrength = 0.87f;    // Unity elevation 8.7 / 10 (scaled)
    float mountainPower = 2.18f;
    float mountainGain = 0.8f;
    float mountainSmoothing = 1.0f;    // Unity: peakSmoothing

    // Mask noise (controls where mountains appear)
    int maskOctaves = 3;
    float maskScale = 1.09f;
    float maskPersistence = 0.55f;
    float maskLacunarity = 1.66f;

    // Ocean parameters
    float oceanDepthMultiplier = 5.0f;
    float oceanFloorDepth = 1.36f;
    float oceanFloorSmoothing = 0.5f;
    float mountainBlend = 1.16f;

    // Overall scaling
    float heightScale = 0.04f;         // Final height displacement scale
    float globalFrequency = 1.0f;      // Multiplier for all noise frequencies (higher = more detail = bigger feeling planet)

    // Vertex perturbation (micro-detail roughness)
    float perturbStrength = 0.002f;    // Strength of high-freq displacement
    float perturbScale = 20.0f;        // Frequency of perturbation noise
};

// Shading noise settings for Earth-like surface detail
struct EarthShadingSettings
{
    // Existing noise scales
    float biomeNoiseScale = 0.5f;   // Large-scale climate zones
    float detailNoiseScale = 2.0f;  // Medium-scale terrain variation
    float fineNoiseScale = 8.0f;    // Fine-scale surface detail
    float warpStrength = 0.3f;      // Domain warping for natural variation

    // Large-scale noise for climate zones
    float largeNoiseScale = 0.3f;
    int largeNoiseOctaves = 3;

    // Small-scale noise (high-frequency detail)
    float smallNoiseScale = 15.0f;
    int smallNoiseOctaves = 5;

    // Color blending parameters
    float flatColBlend = 1.5f;      // Height threshold for low/high gradient blend
    float flatColBlendNoise = 0.3f; // Noise influence on gradient blend
};

// Earth terrain color palette with gradient pairs (matches Unity reference)
struct EarthColors
{
    // Shore (beach) gradient
    glm::vec3 shoreLow = glm::vec3(0.76f, 0.70f, 0.50f);
    glm::vec3 shoreHigh = glm::vec3(0.65f, 0.58f, 0.42f);

    // Flat terrain variant A (plains/grassland)
    glm::vec3 flatLowA = glm::vec3(0.45f, 0.55f, 0.30f);
    glm::vec3 flatHighA = glm::vec3(0.20f, 0.40f, 0.18f);

    // Flat terrain variant B (forest/taiga)
    glm::vec3 flatLowB = glm::vec3(0.15f, 0.40f, 0.15f);
    glm::vec3 flatHighB = glm::vec3(0.10f, 0.28f, 0.12f);

    // Steep terrain (rock/cliff)
    glm::vec3 steepLow = glm::vec3(0.40f, 0.38f, 0.35f);
    glm::vec3 steepHigh = glm::vec3(0.30f, 0.28f, 0.26f);

    // Snow
    glm::vec3 snow = glm::vec3(0.95f, 0.95f, 0.98f);
};

// GPU-accelerated terrain height generation
class TerrainGenerator
{
public:
    TerrainGenerator();
    ~TerrainGenerator();

    TerrainGenerator(const TerrainGenerator&) = delete;
    TerrainGenerator& operator=(const TerrainGenerator&) = delete;

    // Initialize height compute shader
    bool Initialize(const std::string& shaderPath);

    // Initialize both height and shading compute shaders
    bool Initialize(const std::string& heightShaderPath, const std::string& shadingShaderPath);

    // Generate heights using EarthTerrainSettings
    std::vector<float> GenerateHeights(
        const std::vector<glm::vec3>& vertices,
        uint32_t seed,
        const EarthTerrainSettings& settings);

    // Generate heights on GPU (legacy API)
    std::vector<float> GenerateHeights(
        const std::vector<glm::vec3>& vertices,
        uint32_t seed,
        const ComputeNoiseParams& continentParams,
        const ComputeNoiseParams& mountainParams,
        const ComputeNoiseParams& maskParams,
        float oceanDepthMultiplier,
        float oceanFloorDepth,
        float oceanFloorSmoothing,
        float mountainBlend);

    // Generate shading data (biome, detail, temperature, moisture)
    std::vector<glm::vec4> GenerateShadingData(
        const std::vector<glm::vec3>& vertices,
        uint32_t seed,
        const EarthShadingSettings& settings);

    // Check if compute shader is available
    bool IsReady() const { return _computeShader.IsValid(); }

    // Check if shading shader is available
    bool IsShadingReady() const { return _shadingShader.IsValid(); }

private:
    ComputeShader _computeShader;
    ComputeShader _shadingShader;
    GpuBuffer<float> _vertexBuffer;  // Packed as x,y,z floats to avoid alignment issues
    GpuBuffer<float> _heightBuffer;
    GpuBuffer<glm::vec4> _shadingBuffer;  // Output: vec4 per vertex (biome, detail, temp, moisture)
};

} // namespace planets::render
