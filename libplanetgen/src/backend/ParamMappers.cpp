#include "ParamMappers.h"

namespace planetgen
{

HeightParams MakeHeightParams(const PgBodyDesc& desc, uint32_t seed, uint32_t vertexCount)
{
    HeightParams p{};
    p.numVertices = vertexCount;
    p.seed = seed;
    p.useTectonics = desc.use_tectonics;
    p.numPlates = desc.num_plates;
    p.continentalFraction = desc.continental_fraction;
    p.boundaryWidth = desc.boundary_width;
    p.convergentMountainScale = desc.convergent_mountain_scale;
    p.divergentRiftDepth = desc.divergent_rift_depth;
    p.coastlineNoise = desc.coastline_noise;
    p.plateElevationNoise = desc.plate_elevation_noise;
    p.oceanDepthMultiplier = desc.ocean_depth_multiplier;
    p.oceanFloorDepth = desc.ocean_floor_depth;
    p.oceanFloorSmoothing = desc.ocean_floor_smoothing;
    p.mountainBlend = desc.mountain_blend;
    p.heightScale = desc.height_scale;
    p.continentBaseLevel = desc.continent_base_level;
    p.globalFrequency = desc.global_frequency;
    p.normalEpsilon = desc.normal_epsilon;
    p.mountainPower = desc.mountain_power;
    p.mountainGain = desc.mountain_gain;
    p.mountainSmooth = desc.mountain_smoothing;
    p.continentLayers = desc.continent_noise.octaves;
    p.continentScale = desc.continent_noise.scale;
    p.continentPersistence = desc.continent_noise.persistence;
    p.continentLacunarity = desc.continent_noise.lacunarity;
    p.continentMultiplier = desc.continent_noise.strength;
    p.mountainLayers = desc.mountain_noise.octaves;
    p.mountainScale = desc.mountain_noise.scale;
    p.mountainPersistence = desc.mountain_noise.persistence;
    p.mountainLacunarity = desc.mountain_noise.lacunarity;
    p.mountainMultiplier = desc.mountain_noise.strength;
    p.maskLayers = desc.mask_noise.octaves;
    p.maskScale = desc.mask_noise.scale;
    p.maskPersistence = desc.mask_noise.persistence;
    p.maskLacunarity = desc.mask_noise.lacunarity;
    p.maskMultiplier = 1.0f; // always 1 (was hardcoded in old SetHeightUniforms)
    // Offsets are fixed constants from the old implementation
    p.continentOffsetX = 0.0f;    p.continentOffsetY = 0.0f;    p.continentOffsetZ = 0.0f;
    p.mountainOffsetX  = 1000.0f; p.mountainOffsetY  = 1000.0f; p.mountainOffsetZ  = 1000.0f;
    p.maskOffsetX      = 2000.0f; p.maskOffsetY      = 2000.0f; p.maskOffsetZ      = 2000.0f;
    p.detailLowThreshold  = desc.detail_low_threshold;
    p.detailHighThreshold = desc.detail_high_threshold;
    p.perturbStrengthLow  = desc.perturb_strength_low;
    p.perturbStrengthHigh = desc.perturb_strength_high;
    p.detailOctavesLow    = desc.detail_octaves_low;
    p.detailOctavesHigh   = desc.detail_octaves_high;
    p.detailPersistence   = desc.detail_persistence;
    p.detailLacunarity    = desc.detail_lacunarity;
    p.perturbScale        = desc.perturb_scale;
    p.useOceanFloor       = desc.use_ocean_floor;
    p.shelfWidth          = desc.shelf_width;
    p.oceanRidgeOctaves   = desc.ocean_ridge_octaves;
    p.oceanRidgeScale     = desc.ocean_ridge_scale;
    p.oceanRidgeStrength  = desc.ocean_ridge_strength;
    p.oceanRidgePower     = desc.ocean_ridge_power;
    p.oceanRidgeGain      = desc.ocean_ridge_gain;
    p.trenchOctaves       = desc.trench_octaves;
    p.trenchScale         = desc.trench_scale;
    p.trenchDepth         = desc.trench_depth;
    p.abyssalOctaves      = desc.abyssal_octaves;
    p.abyssalScale        = desc.abyssal_scale;
    p.abyssalStrength     = desc.abyssal_strength;
    return p;
}

ShadingEarthParams MakeShadingEarthParams(const PgShadingDesc& desc, uint32_t seed, uint32_t vertexCount)
{
    ShadingEarthParams p{};
    p.numVertices            = vertexCount;
    p.seed                   = seed;
    p.useClimateModel        = desc.use_climate_model;
    p.largeNoiseOctaves      = desc.large_noise_octaves;
    p.largeNoiseScale        = desc.large_noise_scale;
    p.detailNoiseScale       = desc.detail_noise_scale;
    p.smallNoiseScale        = desc.small_noise_scale;
    p.smallNoiseOctaves      = desc.small_noise_octaves;
    p.warpStrength           = desc.warp_strength;
    p.temperatureLapseRate   = desc.temperature_lapse_rate;
    p.temperatureExponent    = desc.temperature_exponent;
    p.moistureNoiseScale     = desc.moisture_noise_scale;
    p.moistureNoiseStrength  = desc.moisture_noise_strength;
    p.hadleyIntensity        = desc.hadley_intensity;
    p.continentalityStrength = desc.continentality_strength;
    p.heightScale            = desc.height_scale;
    return p;
}

ShadingGenericParams MakeShadingGenericParams(const PgShadingDesc& desc, uint32_t seed, uint32_t vertexCount)
{
    ShadingGenericParams p{};
    p.numVertices      = vertexCount;
    p.seed             = seed;
    p.heightScale      = desc.height_scale;
    p.detailNoiseScale = desc.detail_noise_scale;
    p.smallNoiseScale  = desc.small_noise_scale;
    p.smallNoiseOctaves = desc.small_noise_octaves;
    p.warpStrength     = desc.warp_strength;
    return p;
}

ErosionParams MakeErosionParams(const PgErosionDesc& desc, uint32_t vertexCount)
{
    ErosionParams p{};
    p.numVertices     = vertexCount;
    p.gridResolution  = desc.grid_resolution;
    p.thermalRate     = desc.thermal_rate;
    p.thermalThreshold = desc.thermal_threshold;
    p.hydraulicRate   = desc.hydraulic_rate;
    p.depositionRate  = desc.deposition_rate;
    p.evaporationRate = desc.evaporation_rate;
    return p;
}

} // namespace planetgen
