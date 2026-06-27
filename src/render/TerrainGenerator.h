#pragma once

#include "ComputeShader.h"
#include "GpuBuffer.h"
#include "BodyRuntime.h"
#include <glm/glm.hpp>
#include <vector>
#include <cstdint>

namespace planets::render
{

// GPU-accelerated terrain generation for the async LOD pipeline.
// Owns the height/shading/erosion compute shaders and dispatches them per patch
// into LOD-owned buffers, with params bound by a BodyRuntime. This per-patch path
// is app-side by design — the libplanetgen GenerationPipeline is whole-mesh +
// stateless, so the LOD streamer keeps its own dispatch and shares only the param
// mapping (via BodyRuntime -> ParamMappers, the same code the library strategies use).
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

private:
    // Direct GL shaders (async LOD path)
    ComputeShader _computeShader;
    ComputeShader _shadingShader;
    ComputeShader _erosionShader;
};

} // namespace planets::render
