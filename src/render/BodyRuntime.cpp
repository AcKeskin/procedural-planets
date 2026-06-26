#include "BodyRuntime.h"
#include <GL/gl3w.h>
#include <glm/glm.hpp>

// Noise domain offsets — must match what the compute shaders expect (same as old Earth.cpp)
namespace
{
const glm::vec3 ContinentNoiseOffset(0.0f);
const glm::vec3 MountainNoiseOffset(1000.0f);
const glm::vec3 MaskNoiseOffset(2000.0f);
constexpr float HeightRangeMultiplier = 1.5f;
} // namespace

namespace planets::render
{

BodyRuntime::BodyRuntime(planetgen::BodyConfig config, const planetgen::PaletteRegistry& registry)
    : _config(std::move(config))
    , _registry(registry)
{}

void BodyRuntime::SetShapeUniforms(ComputeShader& shader, uint32_t seed) const
{
    const auto& s = _config.shape;
    const auto& t = _config.tectonics;
    const auto& o = _config.oceanFloor;
    const auto& h = _config.heightDetail;

    shader.Use();
    shader.SetUint("seed", seed);

    // Continental noise
    shader.SetVec3("continentOffset", ContinentNoiseOffset);
    shader.SetInt("continentLayers",    s.continentNoise.octaves);
    shader.SetFloat("continentScale",   s.continentNoise.scale);
    shader.SetFloat("continentPersistence", s.continentNoise.persistence);
    shader.SetFloat("continentLacunarity",  s.continentNoise.lacunarity);
    shader.SetFloat("continentMultiplier",  s.continentNoise.strength);

    // Mountain noise
    shader.SetVec3("mountainOffset", MountainNoiseOffset);
    shader.SetInt("mountainLayers",    s.mountainNoise.octaves);
    shader.SetFloat("mountainScale",   s.mountainNoise.scale);
    shader.SetFloat("mountainPersistence", s.mountainNoise.persistence);
    shader.SetFloat("mountainLacunarity",  s.mountainNoise.lacunarity);
    shader.SetFloat("mountainMultiplier",  s.mountainNoise.strength);
    shader.SetFloat("mountainPower",   s.mountainPower);
    shader.SetFloat("mountainGain",    s.mountainGain);
    shader.SetFloat("mountainSmooth",  s.mountainSmoothing);

    // Mask noise
    shader.SetVec3("maskOffset", MaskNoiseOffset);
    shader.SetInt("maskLayers",    s.maskNoise.octaves);
    shader.SetFloat("maskScale",   s.maskNoise.scale);
    shader.SetFloat("maskPersistence", s.maskNoise.persistence);
    shader.SetFloat("maskLacunarity",  s.maskNoise.lacunarity);
    shader.SetFloat("maskMultiplier",  1.0f); // always 1

    // Ocean and terrain blending
    shader.SetFloat("oceanDepthMultiplier", s.oceanDepthMultiplier);
    shader.SetFloat("oceanFloorDepth",      s.oceanFloorDepth);
    shader.SetFloat("oceanFloorSmoothing",  s.oceanFloorSmoothing);
    shader.SetFloat("mountainBlend",        s.mountainBlend);
    shader.SetFloat("heightScale",          s.heightScale);
    shader.SetFloat("continentBaseLevel",   s.continentBaseLevel);
    shader.SetFloat("globalFrequency",      s.globalFrequency);
    shader.SetFloat("normalEpsilon",        s.normalEpsilon);

    // Tectonics
    shader.SetInt("useTectonics",             t.enabled ? 1 : 0);
    shader.SetInt("numPlates",                t.numPlates);
    shader.SetFloat("continentalFraction",    t.continentalFraction);
    shader.SetFloat("boundaryWidth",          t.boundaryWidth);
    shader.SetFloat("convergentMountainScale", t.convergentMountainScale);
    shader.SetFloat("divergentRiftDepth",     t.divergentRiftDepth);
    shader.SetFloat("coastlineNoise",         t.coastlineNoise);
    shader.SetFloat("plateElevationNoise",    t.plateElevationNoise);

    // Height-dependent detail
    shader.SetFloat("detailLowThreshold",  h.detailLowThreshold);
    shader.SetFloat("detailHighThreshold", h.detailHighThreshold);
    shader.SetFloat("perturbStrengthLow",  h.perturbStrengthLow);
    shader.SetFloat("perturbStrengthHigh", h.perturbStrengthHigh);
    shader.SetInt("detailOctavesLow",      h.detailOctavesLow);
    shader.SetInt("detailOctavesHigh",     h.detailOctavesHigh);
    shader.SetFloat("detailPersistence",   h.detailPersistence);
    shader.SetFloat("detailLacunarity",    h.detailLacunarity);
    shader.SetFloat("perturbScale",        h.perturbScale);

    // Continent mask sampler (texture unit 4 — arbitrary free slot for compute)
    if (_continentMaskTexId != 0)
    {
        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_3D, _continentMaskTexId);
        shader.SetInt("continentMask", 4);
        shader.SetInt("continentMaskAvailable", 1);
    }
    else
    {
        shader.SetInt("continentMaskAvailable", 0);
    }

    // Ocean floor topology
    shader.SetInt("useOceanFloor",        o.enabled ? 1 : 0);
    shader.SetFloat("shelfWidth",         o.shelfWidth);
    shader.SetInt("oceanRidgeOctaves",    o.oceanRidgeOctaves);
    shader.SetFloat("oceanRidgeScale",    o.oceanRidgeScale);
    shader.SetFloat("oceanRidgeStrength", o.oceanRidgeStrength);
    shader.SetFloat("oceanRidgePower",    o.oceanRidgePower);
    shader.SetFloat("oceanRidgeGain",     o.oceanRidgeGain);
    shader.SetInt("trenchOctaves",        o.trenchOctaves);
    shader.SetFloat("trenchScale",        o.trenchScale);
    shader.SetFloat("trenchDepth",        o.trenchDepth);
    shader.SetInt("abyssalOctaves",       o.abyssalOctaves);
    shader.SetFloat("abyssalScale",       o.abyssalScale);
    shader.SetFloat("abyssalStrength",    o.abyssalStrength);
}

void BodyRuntime::SetShadingUniforms(ComputeShader& shader, uint32_t seed) const
{
    const auto& sh = _config.shading;

    shader.Use();
    shader.SetUint("seed", seed);

    shader.SetFloat("largeNoiseScale",       sh.largeNoiseScale);
    shader.SetInt("largeNoiseOctaves",       sh.largeNoiseOctaves);
    shader.SetFloat("detailNoiseScale",      sh.detailNoiseScale);
    shader.SetFloat("smallNoiseScale",       sh.smallNoiseScale);
    shader.SetInt("smallNoiseOctaves",       sh.smallNoiseOctaves);
    shader.SetFloat("warpStrength",          sh.warpStrength);

    shader.SetInt("useClimateModel",         sh.useClimateModel ? 1 : 0);
    shader.SetFloat("temperatureLapseRate",  sh.temperatureLapseRate);
    shader.SetFloat("temperatureExponent",   sh.temperatureExponent);
    shader.SetFloat("moistureNoiseScale",    sh.moistureNoiseScale);
    shader.SetFloat("moistureNoiseStrength", sh.moistureNoiseStrength);
    shader.SetFloat("hadleyIntensity",       sh.hadleyIntensity);
    shader.SetFloat("continentalityStrength", sh.continentalityStrength);
    shader.SetFloat("heightScale",           _config.shape.heightScale);

    // Generic shading also reads detailNoiseOctaves (for shading_generic.comp)
    shader.SetInt("detailNoiseOctaves",      sh.smallNoiseOctaves);
}

void BodyRuntime::SetRenderUniforms(Shader& shader) const
{
    const auto& sh = _config.shading;
    const float heightScale = _config.shape.heightScale;

    // Biome controls
    shader.SetInt("uUseBiomes",          sh.biomesEnabled ? 1 : 0);
    shader.SetFloat("uSteepnessThreshold", sh.steepnessThreshold);
    shader.SetFloat("uFlatToSteepBlend",   sh.flatToSteepBlend);
    shader.SetFloat("uSnowLatitude",       sh.snowLatitude);
    shader.SetFloat("uSnowBlend",          sh.snowBlend);
    shader.SetFloat("uSnowLine",           sh.snowLine);
    shader.SetFloat("uShoreHeight",        sh.shoreHeight);
    shader.SetInt("uUseClimateModel",      sh.useClimateModel ? 1 : 0);

    // Earth color palette
    shader.SetVec3("uShoreLow",  sh.colorShoreLow);
    shader.SetVec3("uShoreHigh", sh.colorShoreHigh);
    shader.SetVec3("uFlatLowA",  sh.colorFlatLowA);
    shader.SetVec3("uFlatHighA", sh.colorFlatHighA);
    shader.SetVec3("uFlatLowB",  sh.colorFlatLowB);
    shader.SetVec3("uFlatHighB", sh.colorFlatHighB);
    shader.SetVec3("uSteepLow",  sh.colorSteepLow);
    shader.SetVec3("uSteepHigh", sh.colorSteepHigh);
    shader.SetVec3("uSnowColor", sh.colorSnow);

    // Color blending
    shader.SetFloat("uFlatColBlend",      sh.flatColBlend);
    shader.SetFloat("uFlatColBlendNoise", sh.flatColBlendNoise);

    // Height range for normalization
    float heightMin = 1.0f - heightScale * HeightRangeMultiplier;
    float heightMax = 1.0f + heightScale * HeightRangeMultiplier;
    shader.SetVec2("uHeightMinMax", glm::vec2(heightMin, heightMax));

    shader.SetFloat("uCoastalDepthRange", sh.coastalDepthRange);
}

void BodyRuntime::SetErosionUniforms(ComputeShader& shader, size_t vertexCount, int gridResolution) const
{
    const auto& e = _config.erosion;
    shader.Use();
    shader.SetUint("numVertices",    static_cast<unsigned int>(vertexCount));
    shader.SetInt("gridResolution",  gridResolution);
    shader.SetFloat("thermalRate",       e.thermalRate);
    shader.SetFloat("thermalThreshold",  e.thermalThreshold);
    shader.SetFloat("hydraulicRate",     e.hydraulicRate);
    shader.SetFloat("depositionRate",    e.depositionRate);
    shader.SetFloat("evaporationRate",   e.evaporationRate);
}

void BodyRuntime::EnsurePalette() const
{
    if (_paletteLoaded)
        return;
    _paletteLoaded = true;

    const auto& pg = _registry.Resolve(_config.paletteRef.paletteId);
    if (!pg.IsValid())
        return;

    // Convert PaletteEntry → BiomeEntry
    std::vector<BiomeEntry> entries;
    entries.reserve(pg.entries.size());
    for (const auto& pe : pg.entries)
        entries.push_back({ pe.color, pe.parameter });

    // Reconstruct via internal access (BiomePalette has no public fill ctor — use GetEntries)
    _biomePalette.GetEntries() = std::move(entries);
}

BiomePalette BodyRuntime::LoadBiomePalette() const
{
    EnsurePalette();
    return _biomePalette;
}

} // namespace planets::render
