#pragma once

#include "ComputeShader.h"
#include "GpuBuffer.h"
#include <glm/glm.hpp>
#include <vector>
#include <cstdint>
#include "settings/TerrainSettings.h"
#include "settings/SurfaceSettings.h"

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
