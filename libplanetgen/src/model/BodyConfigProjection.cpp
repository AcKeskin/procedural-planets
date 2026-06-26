#include "BodyConfigProjection.h"
#include <cstring>

namespace planetgen
{

PgBodyDesc ProjectToBodyDesc(const BodyConfig& cfg)
{
    PgBodyDesc d{};
    std::memset(&d, 0, sizeof(d));

    const auto& s = cfg.shape;
    d.continent_noise.scale       = s.continentNoise.scale;
    d.continent_noise.octaves     = s.continentNoise.octaves;
    d.continent_noise.persistence = s.continentNoise.persistence;
    d.continent_noise.lacunarity  = s.continentNoise.lacunarity;
    d.continent_noise.strength    = s.continentNoise.strength;

    d.mountain_noise.scale       = s.mountainNoise.scale;
    d.mountain_noise.octaves     = s.mountainNoise.octaves;
    d.mountain_noise.persistence = s.mountainNoise.persistence;
    d.mountain_noise.lacunarity  = s.mountainNoise.lacunarity;
    d.mountain_noise.strength    = s.mountainNoise.strength;

    d.mask_noise.scale       = s.maskNoise.scale;
    d.mask_noise.octaves     = s.maskNoise.octaves;
    d.mask_noise.persistence = s.maskNoise.persistence;
    d.mask_noise.lacunarity  = s.maskNoise.lacunarity;
    d.mask_noise.strength    = s.maskNoise.strength;

    d.height_scale           = s.heightScale;
    d.ocean_depth_multiplier = s.oceanDepthMultiplier;
    d.ocean_floor_depth      = s.oceanFloorDepth;
    d.ocean_floor_smoothing  = s.oceanFloorSmoothing;
    d.mountain_blend         = s.mountainBlend;
    d.continent_base_level   = s.continentBaseLevel;
    d.global_frequency       = s.globalFrequency;
    d.normal_epsilon         = s.normalEpsilon;
    d.mountain_power         = s.mountainPower;
    d.mountain_gain          = s.mountainGain;
    d.mountain_smoothing     = s.mountainSmoothing;

    const auto& t = cfg.tectonics;
    d.use_tectonics            = t.enabled ? 1 : 0;
    d.num_plates               = t.numPlates;
    d.continental_fraction     = t.continentalFraction;
    d.boundary_width           = t.boundaryWidth;
    d.convergent_mountain_scale = t.convergentMountainScale;
    d.divergent_rift_depth     = t.divergentRiftDepth;
    d.coastline_noise          = t.coastlineNoise;
    d.plate_elevation_noise    = t.plateElevationNoise;

    const auto& o = cfg.oceanFloor;
    d.use_ocean_floor     = o.enabled ? 1 : 0;
    d.shelf_width         = o.shelfWidth;
    d.ocean_ridge_octaves = o.oceanRidgeOctaves;
    d.ocean_ridge_scale   = o.oceanRidgeScale;
    d.ocean_ridge_strength = o.oceanRidgeStrength;
    d.ocean_ridge_power   = o.oceanRidgePower;
    d.ocean_ridge_gain    = o.oceanRidgeGain;
    d.trench_octaves      = o.trenchOctaves;
    d.trench_scale        = o.trenchScale;
    d.trench_depth        = o.trenchDepth;
    d.abyssal_octaves     = o.abyssalOctaves;
    d.abyssal_scale       = o.abyssalScale;
    d.abyssal_strength    = o.abyssalStrength;

    const auto& h = cfg.heightDetail;
    d.detail_low_threshold  = h.detailLowThreshold;
    d.detail_high_threshold = h.detailHighThreshold;
    d.perturb_strength_low  = h.perturbStrengthLow;
    d.perturb_strength_high = h.perturbStrengthHigh;
    d.detail_octaves_low    = h.detailOctavesLow;
    d.detail_octaves_high   = h.detailOctavesHigh;
    d.detail_persistence    = h.detailPersistence;
    d.detail_lacunarity     = h.detailLacunarity;
    d.perturb_scale         = h.perturbScale;

    return d;
}

PgShadingDesc ProjectToShadingDesc(const BodyConfig& cfg)
{
    PgShadingDesc d{};
    std::memset(&d, 0, sizeof(d));

    const auto& sh = cfg.shading;
    d.detail_noise_scale      = sh.detailNoiseScale;
    d.small_noise_scale       = sh.smallNoiseScale;
    d.small_noise_octaves     = sh.smallNoiseOctaves;
    d.warp_strength           = sh.warpStrength;
    d.use_climate_model       = sh.useClimateModel ? 1 : 0;
    d.large_noise_scale       = sh.largeNoiseScale;
    d.large_noise_octaves     = sh.largeNoiseOctaves;
    d.temperature_lapse_rate  = sh.temperatureLapseRate;
    d.temperature_exponent    = sh.temperatureExponent;
    d.moisture_noise_scale    = sh.moistureNoiseScale;
    d.moisture_noise_strength = sh.moistureNoiseStrength;
    d.hadley_intensity        = sh.hadleyIntensity;
    d.continentality_strength = sh.continentalityStrength;
    d.height_scale            = cfg.shape.heightScale; // normalization uses shape scale

    return d;
}

PgErosionDesc ProjectToErosionDesc(const BodyConfig& cfg)
{
    PgErosionDesc d{};
    std::memset(&d, 0, sizeof(d));

    const auto& e = cfg.erosion;
    d.iterations       = e.iterations;
    d.grid_resolution  = e.gridResolution;
    d.thermal_rate     = e.thermalRate;
    d.thermal_threshold = e.thermalThreshold;
    d.hydraulic_rate   = e.hydraulicRate;
    d.deposition_rate  = e.depositionRate;
    d.evaporation_rate = e.evaporationRate;

    return d;
}

} // namespace planetgen
