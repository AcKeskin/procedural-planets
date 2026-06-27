#include "GenerationPipeline.h"
#include "../model/BodyConfigProjection.h"

namespace planetgen
{

GenerationPipeline::GenerationPipeline(IComputeBackend& backend,
                                       const ShaderRegistry& registry,
                                       const PaletteRegistry& palettes)
    : _backend(backend), _registry(registry), _palettes(palettes)
{
}

PipelineResult GenerationPipeline::Generate(const BodyConfig& config,
                                            const float* vertices,
                                            uint32_t vertexCount,
                                            uint32_t seed)
{
    PipelineResult result;
    if (!vertices || vertexCount == 0)
        return result;

    // Project the typed config down to the flat descriptors the strategies bind.
    const PgBodyDesc    bodyDesc    = ProjectToBodyDesc(config);
    const PgShadingDesc shadingDesc = ProjectToShadingDesc(config);
    const PgErosionDesc erosionDesc = ProjectToErosionDesc(config);

    StrategyContext sc{ _backend, _registry, vertexCount, seed };

    // Upload geometry once; all stages read from this SSBO (binding 0).
    const size_t vertexBytes = vertexCount * 3 * sizeof(float);
    const GpuBufferHandle vertexBuf = _backend.CreateBuffer(vertexBytes);
    _backend.UploadBuffer(vertexBuf, vertices, vertexBytes);

    // --- Height (always) ---
    HeightBuffers hb = _height.Run(sc, bodyDesc, vertexBuf);
    GpuBufferHandle heights = hb.heights;
    const GpuBufferHandle normals = hb.normals;

    // --- Erosion (only when the config enables it) ---
    if (config.erosion.enabled)
        heights = _erosion.Run(sc, erosionDesc, heights);

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

} // namespace planetgen
