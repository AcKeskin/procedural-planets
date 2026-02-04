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

// Comprehensive Earth terrain settings
// All parameters exposed for fine-tuning realistic planet surfaces
struct EarthTerrainSettings
{
    // Continental noise (large-scale landmass shapes)
    int continentOctaves = 5;
    float continentScale = 0.8f;       // Low frequency for continent-sized features
    float continentPersistence = 0.55f;
    float continentLacunarity = 2.0f;
    float continentStrength = 1.2f;    // Base height influence

    // Mountain ridge noise (sharp peaks)
    int mountainOctaves = 6;
    float mountainScale = 1.5f;        // Higher frequency for detail
    float mountainPersistence = 0.5f;
    float mountainLacunarity = 2.2f;
    float mountainStrength = 0.8f;
    float mountainPower = 2.5f;        // Sharpness of ridges (higher = sharper)
    float mountainGain = 1.2f;         // Weight propagation
    float mountainSmoothing = 0.4f;    // Anti-aliasing smoothing

    // Mask noise (controls where mountains appear)
    int maskOctaves = 4;
    float maskScale = 0.6f;
    float maskPersistence = 0.5f;
    float maskLacunarity = 2.0f;

    // Ocean parameters
    float oceanDepthMultiplier = 4.0f; // How much deeper oceans are than land is high
    float oceanFloorDepth = 1.2f;      // Clamping floor (prevents unrealistic depths)
    float oceanFloorSmoothing = 0.6f;  // Continental shelf transition smoothness
    float mountainBlend = 1.0f;        // Elevation-based mountain blend zone

    // Overall scaling
    float heightScale = 0.04f;         // Final height displacement scale (4% of radius)
};

// GPU-accelerated terrain height generation
class TerrainGenerator
{
public:
    TerrainGenerator();
    ~TerrainGenerator();

    TerrainGenerator(const TerrainGenerator&) = delete;
    TerrainGenerator& operator=(const TerrainGenerator&) = delete;

    // Initialize compute shader
    bool Initialize(const std::string& shaderPath);

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

    // Check if compute shader is available
    bool IsReady() const { return _computeShader.IsValid(); }

private:
    ComputeShader _computeShader;
    GpuBuffer<float> _vertexBuffer;  // Packed as x,y,z floats to avoid alignment issues
    GpuBuffer<float> _heightBuffer;
};

} // namespace planets::render
