#pragma once

// Bakes the continent mask: runs continent_growth.comp into an RG32F 3D texture
// (R=continentalness, G=accretionAge) that the height stage samples by direction.
// Pure function of (config, seed) — same inputs yield the same volume, which is what
// lets a standalone patch match the whole-mesh region byte-for-byte.

#include "../backend/IComputeBackend.h"
#include "../backend/ShaderRegistry.h"
#include "../model/BodyConfig.h"

#include <cstdint>

namespace planetgen
{

// The image binding the growth compute writes (image3D, binding = 0) and the UBO
// binding it reads params from (binding = 1) — must match continent_growth.comp.
constexpr uint32_t kMaskImageBinding = 0;
constexpr uint32_t kMaskParamBinding = 1;

// Bake the continent mask for (config, seed). Creates + returns an RG32F 3D texture
// handle owned by the caller (the body cache). Returns 0 on failure (program missing).
GpuTextureHandle BakeContinentMask(IComputeBackend& backend,
                                   const ShaderRegistry& registry,
                                   const BodyConfig& config,
                                   uint32_t seed);

// Cache key for the baked mask — what it must be rebuilt on. Mask geometry depends
// on (seed, resolution); the growth params derive from config.shape, but for the
// current model bodies are recreated on config change, so (seed, resolution) is the
// invalidation signal. Pin finer invalidation later if in-place edits need it.
uint64_t MaskCacheKey(const BodyConfig& config, uint32_t seed);

// Return the cached mask for (config, seed), baking (or rebaking on a stale key) as
// needed. `cachedTexture` / `cachedKey` are the body's cache fields, updated in place.
// Returns 0 when the config disables the mask or baking fails.
GpuTextureHandle EnsureContinentMask(IComputeBackend& backend,
                                     const ShaderRegistry& registry,
                                     const BodyConfig& config,
                                     uint32_t seed,
                                     GpuTextureHandle& cachedTexture,
                                     uint64_t& cachedKey);

} // namespace planetgen
