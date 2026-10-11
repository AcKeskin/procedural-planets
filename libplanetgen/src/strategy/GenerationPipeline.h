#pragma once

// The generation orchestrator. Built from a BodyConfig, it runs the canonical
// pass sequence height -> [erosion] -> shading through the strategy interfaces
// (built-ins unless the caller injects its own), threading buffers between stages. Stateless and synchronous per call:
// (config, seed, geometry, backend) -> per-vertex data + palette. LOD/streaming
// stays app-side and is no concern of the pipeline.
//
// Closed extension: a new stage is a new IGenerationStrategy impl + shader +
// config block. Swapping a strategy impl is a constructor argument; it needs no
// edit to sibling strategies or the orchestrator.

#include "IGenerationStrategy.h"
#include "ContinentMaskPass.h"
#include "MacroWorldPass.h"
#include "planetgen/model/BodyConfig.h"
#include "planetgen/model/PaletteRegistry.h"

#include <cstdint>
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

// Optional continent-mask cache, owned by the body and threaded into a call.
// When non-null, the pipeline ensures (bakes/reuses) the mask and binds it for
// the height stage. Null = no mask (e.g. the standalone per-stage ABI calls).
// Namespace scope (not nested) so `= {}` default arguments compile on GCC/Clang.
struct PipelineMaskCache
{
    GpuTextureHandle* texture = nullptr; // body's cached handle (inout)
    uint64_t*         key     = nullptr; // body's cache key (inout)
};

// Optional macro world cache, owned by the context (shared by its bodies). When
// non-null and the config enables the layer, the pipeline bakes/reuses the
// field (it needs the mask) and binds it for height + shading.
struct PipelineMacroCache
{
    GpuTextureHandle* texture = nullptr; // body's cached handle (inout)
    uint64_t*         key     = nullptr; // body's cache key (inout)
};

class GenerationPipeline
{
public:
    GenerationPipeline(IComputeBackend& backend,
                       const ShaderRegistry& registry,
                       const PaletteRegistry& palettes,
                       GenerationStrategies strategies = {});

    using MaskCache  = PipelineMaskCache;
    using MacroCache = PipelineMacroCache;

    // Run the full pass sequence from a config. Enables erosion only when the
    // config's erosion block is on. `vertices` = packed float3 (x,y,z per vertex).
    PipelineResult Generate(const BodyConfig& config,
                            const float* vertices,
                            uint32_t vertexCount,
                            uint32_t seed,
                            MaskCache mask = {},
                            MacroCache macro = {});

    // Per-patch path: run height -> shading writing into the caller-owned output
    // buffers (GL handles), GPU-resident, with NO CPU readback. Erosion is skipped
    // (neighbour-coupled — would seam across patch boundaries; out of scope here).
    // The library allocates no output and creates no fence: it records the dispatch
    // and returns. The caller owns sync (drops/polls its own fence) and owns the
    // output buffers (the pipeline never destroys them). Returns true on dispatch.
    bool GeneratePatch(const BodyConfig& config,
                       const float* vertices,
                       uint32_t vertexCount,
                       uint32_t seed,
                       GpuBufferHandle outHeights,
                       GpuBufferHandle outNormals,
                       GpuBufferHandle outShading,
                       MaskCache mask = {},
                       MacroCache macro = {});

    // Height stage alone (pg_generate_heights), with the same mask and macro field as
    // the patch path. Returns GPU buffers the caller owns.
    HeightBuffers GenerateHeights(const BodyConfig& config,
                                  const float* vertices,
                                  uint32_t vertexCount,
                                  uint32_t seed,
                                  MaskCache mask = {},
                                  MacroCache macro = {});

    // Shading stage alone over caller heights (pg_generate_shading), with the body's
    // macro field. `desc` is the caller's; the config only picks the mask and field.
    GpuBufferHandle GenerateShading(const BodyConfig& config,
                                    const PgShadingDesc& desc,
                                    const float* vertices,
                                    const float* heights,
                                    uint32_t vertexCount,
                                    uint32_t seed,
                                    MaskCache mask = {},
                                    MacroCache macro = {});

private:
    IComputeBackend& _backend;
    const ShaderRegistry& _registry;
    const PaletteRegistry& _palettes;

    std::unique_ptr<IHeightStrategy>  _height;
    std::unique_ptr<IErosionStrategy> _erosion;
    std::unique_ptr<IShadingStrategy> _shading;
};

} // namespace planetgen
