#pragma once

#include "ComputeShader.h"
#include "GpuBuffer.h"
#include "CelestialBody.h"
#include <planetgen/planetgen.h>
#include <glm/glm.hpp>
#include <vector>
#include <cstdint>
#include "settings/TerrainSettings.h"
#include "settings/SurfaceSettings.h"

namespace planets::render
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
// Synchronous paths delegate to libplanetgen; async dispatch uses direct GL for LOD pipeline.
class TerrainGenerator
{
public:
    TerrainGenerator();
    ~TerrainGenerator();

    TerrainGenerator(const TerrainGenerator&) = delete;
    TerrainGenerator& operator=(const TerrainGenerator&) = delete;

    // Initialize height compute shader
    bool Initialize(const std::string& shaderPath);

    // Initialize height, shading, and erosion compute shaders
    bool Initialize(const std::string& heightShaderPath,
                    const std::string& shadingShaderPath,
                    const std::string& erosionShaderPath);

    // Initialize libplanetgen context (uses existing GL context on calling thread)
    bool InitializeLibrary();

    // Generate heights and analytical normals via libplanetgen
    std::vector<float> GenerateHeights(const std::vector<glm::vec3>& vertices,
                                       uint32_t seed,
                                       const EarthTerrainSettings& settings,
                                       std::vector<glm::vec3>* outNormals = nullptr);

    // Generate heights on GPU (legacy API)
    std::vector<float> GenerateHeights(const std::vector<glm::vec3>& vertices,
                                       uint32_t seed,
                                       const ComputeNoiseParams& continentParams,
                                       const ComputeNoiseParams& mountainParams,
                                       const ComputeNoiseParams& maskParams,
                                       float oceanDepthMultiplier,
                                       float oceanFloorDepth,
                                       float oceanFloorSmoothing,
                                       float mountainBlend);

    // Generate shading data via libplanetgen
    std::vector<glm::vec4>
    GenerateShadingData(const std::vector<glm::vec3>& vertices, uint32_t seed, const EarthShadingSettings& settings);

    // Abstract dispatch: body type sets its own uniforms (async LOD path)
    void DispatchHeightsAsync(GpuBuffer<float>& vertexBuffer,
                              GpuBuffer<float>& heightBuffer,
                              GpuBuffer<float>& normalBuffer,
                              size_t vertexCount,
                              const CelestialBody& body,
                              uint32_t seed);

    // Abstract dispatch: body type sets its own uniforms (async LOD path)
    void DispatchShadingAsync(GpuBuffer<float>& vertexBuffer,
                              GpuBuffer<glm::vec4>& shadingBuffer,
                              GpuBuffer<float>& heightBuffer,
                              size_t vertexCount,
                              const CelestialBody& body,
                              uint32_t seed);

    // Earth-specific dispatch (legacy, used by synchronous paths)
    void DispatchHeightsAsync(GpuBuffer<float>& vertexBuffer,
                              GpuBuffer<float>& heightBuffer,
                              GpuBuffer<float>& normalBuffer,
                              size_t vertexCount,
                              uint32_t seed,
                              const EarthTerrainSettings& settings);

    // Earth-specific dispatch (legacy, used by synchronous paths)
    void DispatchShadingAsync(GpuBuffer<float>& vertexBuffer,
                              GpuBuffer<glm::vec4>& shadingBuffer,
                              GpuBuffer<float>& heightBuffer,
                              size_t vertexCount,
                              uint32_t seed,
                              const EarthShadingSettings& settings,
                              float heightScale);

    // Abstract erosion dispatch: body sets its own uniforms
    void DispatchErosionAsync(GpuBuffer<float>& inputBuffer,
                              GpuBuffer<float>& outputBuffer,
                              size_t vertexCount,
                              int gridResolution,
                              const CelestialBody& body);

    // Earth-specific erosion dispatch (legacy)
    void DispatchErosionAsync(GpuBuffer<float>& inputBuffer,
                              GpuBuffer<float>& outputBuffer,
                              size_t vertexCount,
                              int gridResolution,
                              const EarthTerrainSettings& settings);

    bool IsReady() const { return _computeShader.IsValid() || _pgContext != nullptr; }
    bool IsShadingReady() const { return _shadingShader.IsValid() || _pgContext != nullptr; }
    bool IsErosionReady() const { return _erosionShader.IsValid(); }
    bool IsLibraryReady() const { return _pgContext != nullptr; }

private:
    void SetHeightUniforms(uint32_t seed, size_t vertexCount, const EarthTerrainSettings& settings);
    void SetShadingUniforms(uint32_t seed, size_t vertexCount, const EarthShadingSettings& settings, float heightScale);
    void SetErosionUniforms(size_t vertexCount, int gridResolution, const EarthTerrainSettings& settings);

    // Convert EarthTerrainSettings to PgBodyDesc
    static PgBodyDesc ToPgBodyDesc(const EarthTerrainSettings& settings);
    // Convert EarthShadingSettings to PgShadingDesc
    static PgShadingDesc ToPgShadingDesc(const EarthShadingSettings& settings, float heightScale);

    // Direct GL shaders (async LOD path)
    ComputeShader _computeShader;
    ComputeShader _shadingShader;
    ComputeShader _erosionShader;
    GpuBuffer<float> _vertexBuffer;
    GpuBuffer<float> _heightBuffer;
    GpuBuffer<float> _normalBuffer;
    GpuBuffer<glm::vec4> _shadingBuffer;

    // libplanetgen handles (synchronous path)
    PgContext _pgContext = nullptr;
};

} // namespace planets::render
