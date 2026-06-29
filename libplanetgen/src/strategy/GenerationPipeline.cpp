#include "GenerationPipeline.h"
#include "ContinentMaskPass.h"
#include "../model/BodyConfigProjection.h"

namespace planetgen
{

namespace
{
// Resolve the continent mask for this call from the body's cache (bake/reuse),
// or 0 when no cache was supplied or the config disables it.
GpuTextureHandle ResolveMask(IComputeBackend& backend, const ShaderRegistry& registry,
                             const BodyConfig& config, uint32_t seed,
                             GenerationPipeline::MaskCache mask)
{
    if (!mask.texture || !mask.key)
        return 0;
    return EnsureContinentMask(backend, registry, config, seed, *mask.texture, *mask.key);
}
} // namespace

GenerationPipeline::GenerationPipeline(IComputeBackend& backend,
                                       const ShaderRegistry& registry,
                                       const PaletteRegistry& palettes)
    : _backend(backend), _registry(registry), _palettes(palettes)
{
}

PipelineResult GenerationPipeline::Generate(const BodyConfig& config,
                                            const float* vertices,
                                            uint32_t vertexCount,
                                            uint32_t seed,
                                            MaskCache mask)
{
    PipelineResult result;
    if (!vertices || vertexCount == 0)
        return result;

    // Project the typed config down to the flat descriptors the strategies bind.
    const PgBodyDesc    bodyDesc    = ProjectToBodyDesc(config);
    const PgShadingDesc shadingDesc = ProjectToShadingDesc(config);
    const PgErosionDesc erosionDesc = ProjectToErosionDesc(config);

    StrategyContext sc{ _backend, _registry, vertexCount, seed };
    sc.continentMask = ResolveMask(_backend, _registry, config, seed, mask);

    // Upload geometry once; all stages read from this SSBO (binding 0).
    const size_t vertexBytes = vertexCount * 3 * sizeof(float);
    const GpuBufferHandle vertexBuf = _backend.CreateBuffer(vertexBytes);
    _backend.UploadBuffer(vertexBuf, vertices, vertexBytes);

    // --- Height (always) ---
    HeightBuffers hb = _height.Run(sc, bodyDesc, vertexBuf);
    GpuBufferHandle heights = hb.heights;
    const GpuBufferHandle normals = hb.normals;

    // --- Erosion (only when the config enables it) ---
    // Erosion may return its own scratch buffer instead of the input; if so, the
    // original height SSBO is no longer referenced and must be freed here.
    if (config.erosion.enabled)
    {
        const GpuBufferHandle preErosion = heights;
        heights = _erosion.Run(sc, erosionDesc, heights);
        if (heights != preErosion)
            _backend.DestroyBuffer(preErosion);
    }

    // --- Shading (always) ---
    const GpuBufferHandle shading = _shading.Run(sc, shadingDesc, vertexBuf, heights);

    // --- Palette (pure lookup) ---
    result.palette = _palette.Resolve(_palettes, config.paletteRef.paletteId);

    // Read back to CPU.
    result.count = vertexCount;
    result.heights.resize(vertexCount);
    result.normals.resize(vertexCount * 3);
    result.shading.resize(vertexCount * 4);

    if (heights)
        _backend.DownloadBuffer(heights, result.heights.data(), vertexCount * sizeof(float));
    if (normals)
        _backend.DownloadBuffer(normals, result.normals.data(), vertexCount * 3 * sizeof(float));
    if (shading)
        _backend.DownloadBuffer(shading, result.shading.data(), vertexCount * 4 * sizeof(float));

    result.heightsGpuBuffer = heights;
    result.normalsGpuBuffer = normals;
    result.shadingGpuBuffer = shading;

    // The vertex SSBO is scratch — stages copied what they needed.
    _backend.DestroyBuffer(vertexBuf);

    return result;
}

bool GenerationPipeline::GeneratePatch(const BodyConfig& config,
                                       const float* vertices,
                                       uint32_t vertexCount,
                                       uint32_t seed,
                                       GpuBufferHandle outHeights,
                                       GpuBufferHandle outNormals,
                                       GpuBufferHandle outShading,
                                       MaskCache mask)
{
    if (!vertices || vertexCount == 0 || !outHeights || !outNormals || !outShading)
        return false;

    const PgBodyDesc    bodyDesc    = ProjectToBodyDesc(config);
    const PgShadingDesc shadingDesc = ProjectToShadingDesc(config);

    // Same per-vertex strategies as the whole-mesh path, but writing into the
    // caller's buffers (outXxx) — this is what makes a standalone patch
    // byte-identical to the whole-mesh region (cross-consistency). The mask is the
    // same body-cached texture the whole-mesh path samples (per-vertex-pure lookup).
    StrategyContext sc{ _backend, _registry, vertexCount, seed };
    sc.outHeights = outHeights;
    sc.outNormals = outNormals;
    sc.outShading = outShading;
    sc.continentMask = ResolveMask(_backend, _registry, config, seed, mask);

    // Upload geometry once; all stages read from this SSBO (binding 0).
    const size_t vertexBytes = vertexCount * 3 * sizeof(float);
    const GpuBufferHandle vertexBuf = _backend.CreateBuffer(vertexBytes);
    _backend.UploadBuffer(vertexBuf, vertices, vertexBytes);

    // Height -> shading, no erosion. The strategies bind sc.outXxx in place.
    const HeightBuffers hb = _height.Run(sc, bodyDesc, vertexBuf);
    _shading.Run(sc, shadingDesc, vertexBuf, hb.heights);

    // No DownloadBuffer, no fence: the data is GPU-resident in the caller's
    // buffers and the caller owns completion tracking. Only the scratch vertex
    // SSBO is ours to free; the output buffers belong to the caller.
    _backend.DestroyBuffer(vertexBuf);

    return true;
}

} // namespace planetgen
