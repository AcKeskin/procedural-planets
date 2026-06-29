#include "ContinentMaskPass.h"
#include "../backend/ParamBlocks.h"

#include <algorithm>

namespace planetgen
{

GpuTextureHandle BakeContinentMask(IComputeBackend& backend,
                                   const ShaderRegistry& registry,
                                   const BodyConfig& config,
                                   uint32_t seed)
{
    const uint32_t program = registry.Resolve(BuiltinProgram::ContinentGrowth);
    if (!program)
        return 0;

    const auto& sh = config.shape;
    const uint32_t res = static_cast<uint32_t>(
        std::clamp(sh.continentMaskResolution, 8, 256));

    const GpuTextureHandle mask = backend.CreateTexture3D_RG32F(res);

    // Params — the same canonical std140 struct the app used (ParamBlocks.h).
    ContinentGrowthParams p{};
    p.count        = static_cast<uint32_t>(std::max(1, sh.continentCount));
    p.seed         = seed;
    p.resolution   = res;
    p.warpOctaves  = 3u; // coastline-detail octaves
    p.sizeVariance = sh.continentSizeVariance;
    p.clustering   = sh.continentClustering;

    backend.BindShader(program);
    backend.BindImage3D(mask, kMaskImageBinding);

    const GpuBufferHandle ubo = backend.CreateParamBuffer(sizeof(p));
    backend.SetParams(ubo, &p, sizeof(p), kMaskParamBinding);

    // local_size = 8^3 in the shader -> ceil(res/8) groups per axis.
    const uint32_t groups = (res + 7u) / 8u;
    backend.Dispatch(groups, groups, groups);

    // Mask must be visible to the height stage's sampler before it runs.
    backend.ImageBarrier();

    backend.DestroyParamBuffer(ubo);

    return mask;
}

uint64_t MaskCacheKey(const BodyConfig& config, uint32_t seed)
{
    const uint32_t res = static_cast<uint32_t>(
        std::clamp(config.shape.continentMaskResolution, 8, 256));
    return (static_cast<uint64_t>(seed) << 32) | static_cast<uint64_t>(res);
}

GpuTextureHandle EnsureContinentMask(IComputeBackend& backend,
                                     const ShaderRegistry& registry,
                                     const BodyConfig& config,
                                     uint32_t seed,
                                     GpuTextureHandle& cachedTexture,
                                     uint64_t& cachedKey)
{
    // The mask only matters when tectonics drives continents (the height shader
    // gates the mask path on useTectonics). Disabled -> no mask.
    if (!config.tectonics.enabled)
        return 0;

    const uint64_t key = MaskCacheKey(config, seed);
    if (cachedTexture != 0 && cachedKey == key)
        return cachedTexture; // cache hit

    // Miss/stale: free the cached texture before rebaking so it doesn't leak.
    if (cachedTexture != 0)
        backend.DestroyTexture(cachedTexture);

    cachedTexture = BakeContinentMask(backend, registry, config, seed);
    cachedKey = (cachedTexture != 0) ? key : 0;
    return cachedTexture;
}

} // namespace planetgen
