#pragma once

// Per-stage generation strategy interfaces.
//
// Each stage is a dumb, stateless step: it declares the shader it needs (by
// BuiltinProgram id), builds its std140 param block, binds it through the
// backend, and asks the backend to dispatch. No GL types, no glDispatchCompute,
// no program compilation live here — those are the backend's job (compute-backend
// owns GPU mechanics). The GenerationPipeline orchestrator (see GenerationPipeline.h)
// composes these from a BodyConfig and threads buffers between them.
//
// Adding a stage is a closed change: implement the relevant interface, register a
// shader, add a config block — with no edit to sibling strategies or the
// orchestrator core.

#include "planetgen/planetgen.h"
#include "../backend/IComputeBackend.h"
#include "../backend/ShaderRegistry.h"
#include "../model/PaletteRegistry.h"

#include <cstdint>

namespace planetgen
{

// The seam every GPU strategy runs against. Bundles the backend (GPU mechanics),
// the shader registry (id -> compiled program), and the per-call geometry inputs.
// The orchestrator builds one of these and hands it to each stage.
//
// Optional output handles (per-patch path): when a stage's outXxx field is
// non-zero, the stage binds that caller-owned buffer as its output instead of
// allocating its own — the write-in-place mode the public per-patch call needs.
// Zero means "allocate and return a fresh buffer" (the whole-mesh default). The
// buffers are bound via the backend's existing BindBuffer; the stage never
// destroys a caller-supplied handle.
struct StrategyContext
{
    IComputeBackend& backend;
    const ShaderRegistry& registry;
    uint32_t vertexCount = 0;
    uint32_t seed = 0;

    // Caller-supplied output buffers (0 = strategy allocates its own).
    GpuBufferHandle outHeights = 0;
    GpuBufferHandle outNormals = 0;
    GpuBufferHandle outShading = 0;

    // Continent-mask 3D texture (0 = none). When set, the height stage binds it to
    // the mask sampler unit and flags continentMaskAvailable in its params.
    GpuTextureHandle continentMask = 0;
};

// Output handle bundle for the height stage (heights + packed-float3 normals).
struct HeightBuffers
{
    GpuBufferHandle heights = 0;
    GpuBufferHandle normals = 0;
};

// IHeightStrategy — consumes Shape/Tectonics/OceanFloor/HeightDetail (carried by
// the flat PgBodyDesc the orchestrator projects) + geometry -> heights + normals.
class IHeightStrategy
{
public:
    virtual ~IHeightStrategy() = default;

    // vertices = packed float3 SSBO already uploaded by the caller (binding 0).
    // Returns freshly created heights+normals SSBOs (the caller owns their lifetime).
    virtual HeightBuffers Run(const StrategyContext& sc,
                              const PgBodyDesc& desc,
                              GpuBufferHandle vertices) = 0;
};

// IErosionStrategy — consumes the Erosion block + heights -> modified heights.
// Optional stage: the orchestrator only runs it when the erosion block is enabled.
class IErosionStrategy
{
public:
    virtual ~IErosionStrategy() = default;

    // heights = SSBO from the height stage (binding 0). Returns the eroded heights
    // SSBO (may be the same handle or a new one — caller treats the return as the
    // authoritative heights buffer afterwards).
    virtual GpuBufferHandle Run(const StrategyContext& sc,
                                const PgErosionDesc& desc,
                                GpuBufferHandle heights) = 0;
};

// IShadingStrategy — consumes the Shading/Climate block + geometry + heights ->
// shading vec4. The earth/generic shader choice lives inside the impl.
class IShadingStrategy
{
public:
    virtual ~IShadingStrategy() = default;

    // vertices (binding 0) + heights (binding 2) already uploaded. Returns the
    // shading vec4 SSBO (caller owns its lifetime).
    virtual GpuBufferHandle Run(const StrategyContext& sc,
                                const PgShadingDesc& desc,
                                GpuBufferHandle vertices,
                                GpuBufferHandle heights) = 0;
};

// IPaletteProvider — resolves the body's palette by id. Pure lookup, no GPU work.
class IPaletteProvider
{
public:
    virtual ~IPaletteProvider() = default;

    virtual const Palette& Resolve(const PaletteRegistry& registry,
                                   const std::string& paletteId) const = 0;
};

} // namespace planetgen
