#pragma once

#include "../noise/Noise.h"
#include "../../render/ComputeShader.h"
#include "../../render/GpuBuffer.h"
#include <glm/glm.hpp>
#include <vector>
#include <cstdint>

namespace planets::core
{

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

    // Initialize compute shader
    bool Initialize(const std::string& shaderPath);

    // Generate heights on GPU
    std::vector<float> GenerateHeights(const std::vector<glm::vec3>& vertices,
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
    render::ComputeShader _computeShader;
    render::GpuBuffer<glm::vec3> _vertexBuffer;
    render::GpuBuffer<float> _heightBuffer;
};

} // namespace planets::core
