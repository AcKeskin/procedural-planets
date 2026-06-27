#pragma once

// The generation orchestrator. Built from a BodyConfig, it owns the fixed
// built-in strategy set and the canonical pass sequence height -> [erosion] ->
// shading, threading buffers between stages. Stateless and synchronous per call:
// (config, seed, geometry, backend) -> per-vertex data + palette. LOD/streaming
// stays app-side and is no concern of the pipeline.
//
// Closed extension: a new stage is a new IGenerationStrategy impl + shader +
// config block. The orchestrator composes through the interfaces; swapping a
// strategy impl needs no edit to sibling strategies.

#include "IGenerationStrategy.h"
#include "HeightStrategy.h"
#include "ShadingStrategy.h"
#include "ErosionStrategy.h"
#include "PaletteProvider.h"
#include "../model/BodyConfig.h"
#include "../model/PaletteRegistry.h"

#include <vector>

namespace planetgen
{

// CPU-side readback of one full generation pass plus the resolved palette.
struct PipelineResult
{
    uint32_t count = 0;
    std::vector<float> heights; // count
    std::vector<float> normals; // count * 3 (packed float3)
    std::vector<float> shading; // count * 4 (packed float4)
    Palette palette;            // resolved by id

    // GPU buffer handles (valid while the backend/context lives).
    GpuBufferHandle heightsGpuBuffer = 0;
    GpuBufferHandle normalsGpuBuffer = 0;
    GpuBufferHandle shadingGpuBuffer = 0;
};

class GenerationPipeline
{
public:
    GenerationPipeline(IComputeBackend& backend,
                       const ShaderRegistry& registry,
                       const PaletteRegistry& palettes);

    // Run the full pass sequence from a config. Enables erosion only when the
    // config's erosion block is on. `vertices` = packed float3 (x,y,z per vertex).
    PipelineResult Generate(const BodyConfig& config,
                            const float* vertices,
                            uint32_t vertexCount,
                            uint32_t seed);

private:
    IComputeBackend& _backend;
    const ShaderRegistry& _registry;
    const PaletteRegistry& _palettes;

    // Fixed built-in strategy set.
    HeightStrategy  _height;
    ErosionStrategy _erosion;
    ShadingStrategy _shading;
    PaletteProvider _palette;
};

} // namespace planetgen
