#pragma once

// Bakes the macro world field once per body: plates -> base elevation, hydraulic
// erosion, rivers, moisture, biomes, on a coarse cube-sphere grid (6 faces x N^2,
// see shaders/includes/macro_grid.glsl). Height and shading sample the result by
// direction, so per-patch generation stays neighbour-independent and seam-free.
// Pure function of (config, seed, continent mask) - the cache is an optimisation.
//
// Final field (RGBA32F 2D array, layer = cube face):
//   R = elevation in continentShape units (0 = sea level), eroded + drainage-filled
//   G = river flux (normalised log of upstream cell count)
//   B = moisture [0, 1]
//   A = biome id (MacroBiome)
//
// Bindings shared by the bake passes: images 0-3 (per-pass, documented in each
// .comp), continent mask sampler 4, macro params UBO 3.

#include "../backend/IComputeBackend.h"
#include "../backend/ParamBlocks.h"
#include "../backend/ShaderRegistry.h"
#include "../model/BodyConfig.h"

#include <cstdint>

namespace planetgen
{

// Sampler unit the height + shading stages read the finished field from.
constexpr uint32_t kMacroFieldSamplerUnit = 5;
// Sampler unit the plate stage reads the continent mask from (same as height).
constexpr uint32_t kMacroMaskSamplerUnit = 4;
// UBO binding of MacroParams in every bake pass.
constexpr uint32_t kMacroParamBinding = 3;

// Bake elevations are meters so the ported erosion constants keep their meaning.
constexpr float kMacroMetersPerUnit = 4000.0f;
// Planet radius the erosion's physical cell size is computed for (Earth).
constexpr float kMacroPlanetRadiusMeters = 6371000.0f;

// Clamped grid resolution (cells per face edge) for a config.
uint32_t MacroResolution(const BodyConfig& config);

// Std140 params every bake pass reads, built from the config.
MacroParams MakeMacroParams(const BodyConfig& config, uint32_t seed);

// Runs the bake stages. Owns every texture it creates until handed out with
// TakeField(); the destructor frees whatever it still owns. Stages are public so
// tests can check each one in isolation.
class MacroWorldBake
{
public:
    MacroWorldBake(IComputeBackend& backend, const ShaderRegistry& registry,
                   const BodyConfig& config, uint32_t seed);
    ~MacroWorldBake();

    MacroWorldBake(const MacroWorldBake&) = delete;
    MacroWorldBake& operator=(const MacroWorldBake&) = delete;

    // Stage 1. Writes the geo texture (R = elevation m, G = bedrock, B = sediment,
    // A = water) from plates + the continent mask. False if the program is missing.
    bool Plates(GpuTextureHandle continentMask);

    // Stage 2. Hydraulic erosion on the geo texture, `iterations` rounds of
    // rainfall -> flow -> sediment. Total height + sediment is conserved.
    bool Erode(int iterations);

    // Run every stage. Returns false (and leaves no field) on a missing program.
    bool RunAll(GpuTextureHandle continentMask);

    // Hand the finished field to the caller (the body cache). 0 if not baked.
    GpuTextureHandle TakeField();

    uint32_t Resolution() const { return _res; }
    GpuTextureHandle Geo() const { return _geo; }

private:
    GpuTextureHandle NewArray();
    void Dispatch(uint32_t program, uint32_t passIndex);

    IComputeBackend& _backend;
    const ShaderRegistry& _registry;
    MacroParams _params{};
    int _erosionIterations = 0;
    uint32_t _res = 0;
    GpuBufferHandle _ubo = 0;

    GpuTextureHandle _geo = 0;
    GpuTextureHandle _geoTmp = 0; // erosion ping-pong partner of _geo
    GpuTextureHandle _flux = 0;   // erosion outflow accumulator (R)
    GpuTextureHandle _field = 0;
};

// Bake the full field. Returns a texture owned by the caller, or 0 on failure.
GpuTextureHandle BakeMacroWorld(IComputeBackend& backend, const ShaderRegistry& registry,
                                const BodyConfig& config, uint32_t seed,
                                GpuTextureHandle continentMask);

// What the baked field must be rebuilt on: seed, every macro param, and the
// upstream params the bake reads (continent mask, tectonics, continent noise).
uint64_t MacroCacheKey(const BodyConfig& config, uint32_t seed);

// Return the cached field for (config, seed), baking or rebaking as needed.
// Returns 0 when the layer is off, there is no continent mask, or baking fails.
GpuTextureHandle EnsureMacroWorld(IComputeBackend& backend, const ShaderRegistry& registry,
                                  const BodyConfig& config, uint32_t seed,
                                  GpuTextureHandle continentMask,
                                  GpuTextureHandle& cachedTexture, uint64_t& cachedKey);

} // namespace planetgen
