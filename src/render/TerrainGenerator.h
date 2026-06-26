#pragma once

#include "ComputeShader.h"
#include "GpuBuffer.h"
#include "BodyRuntime.h"
#include <planetgen/planetgen.h>
#include <glm/glm.hpp>
#include <vector>
#include <cstdint>

namespace planets::render
{

// GPU-accelerated terrain generation for the async LOD pipeline.
// Owns the height/shading/erosion compute shaders and dispatches them per patch,
// driven by a BodyRuntime. Also holds the libplanetgen context for the future
// strategy-tier integration (kept available; the sync generation paths were removed).
class TerrainGenerator
{
public:
    TerrainGenerator();
    ~TerrainGenerator();

    TerrainGenerator(const TerrainGenerator&) = delete;
    TerrainGenerator& operator=(const TerrainGenerator&) = delete;

    // Initialize height, shading, and erosion compute shaders
    bool Initialize(const std::string& heightShaderPath,
                    const std::string& shadingShaderPath,
                    const std::string& erosionShaderPath);

    // Initialize libplanetgen context (uses existing GL context on calling thread)
    bool InitializeLibrary();

    // Dispatch via BodyRuntime (async LOD path)
    void DispatchHeightsAsync(GpuBuffer<float>& vertexBuffer,
                              GpuBuffer<float>& heightBuffer,
                              GpuBuffer<float>& normalBuffer,
                              size_t vertexCount,
                              const BodyRuntime& body,
                              uint32_t seed);

    void DispatchShadingAsync(GpuBuffer<float>& vertexBuffer,
                              GpuBuffer<glm::vec4>& shadingBuffer,
                              GpuBuffer<float>& heightBuffer,
                              size_t vertexCount,
                              const BodyRuntime& body,
                              uint32_t seed);

    void DispatchErosionAsync(GpuBuffer<float>& inputBuffer,
                              GpuBuffer<float>& outputBuffer,
                              size_t vertexCount,
                              int gridResolution,
                              const BodyRuntime& body);

    bool IsReady() const { return _computeShader.IsValid(); }
    bool IsShadingReady() const { return _shadingShader.IsValid(); }
    bool IsErosionReady() const { return _erosionShader.IsValid(); }
    bool IsLibraryReady() const { return _pgContext != nullptr; }

private:
    // Direct GL shaders (async LOD path)
    ComputeShader _computeShader;
    ComputeShader _shadingShader;
    ComputeShader _erosionShader;

    // libplanetgen context — kept for the future strategy-tier integration
    PgContext _pgContext = nullptr;
};

} // namespace planets::render
