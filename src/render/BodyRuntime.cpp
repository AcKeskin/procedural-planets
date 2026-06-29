#include "BodyRuntime.h"
#include <glm/glm.hpp>

namespace planets::render
{

BodyRuntime::BodyRuntime(planetgen::BodyConfig config, const planetgen::PaletteRegistry& registry)
    : _config(std::move(config))
    , _registry(registry)
{
}

void BodyRuntime::SetRenderUniforms(Shader& shader) const
{
    const auto& sh = _config.shading;
    const float heightScale = _config.shape.heightScale;
    constexpr float HeightRangeMultiplier = 1.5f;

    // Biome controls
    shader.SetInt("uUseBiomes", sh.biomesEnabled ? 1 : 0);
    shader.SetFloat("uSteepnessThreshold", sh.steepnessThreshold);
    shader.SetFloat("uFlatToSteepBlend", sh.flatToSteepBlend);
    shader.SetFloat("uSnowLatitude", sh.snowLatitude);
    shader.SetFloat("uSnowBlend", sh.snowBlend);
    shader.SetFloat("uSnowLine", sh.snowLine);
    shader.SetFloat("uShoreHeight", sh.shoreHeight);
    shader.SetInt("uUseClimateModel", sh.useClimateModel ? 1 : 0);

    // Earth color palette
    shader.SetVec3("uShoreLow", sh.colorShoreLow);
    shader.SetVec3("uShoreHigh", sh.colorShoreHigh);
    shader.SetVec3("uFlatLowA", sh.colorFlatLowA);
    shader.SetVec3("uFlatHighA", sh.colorFlatHighA);
    shader.SetVec3("uFlatLowB", sh.colorFlatLowB);
    shader.SetVec3("uFlatHighB", sh.colorFlatHighB);
    shader.SetVec3("uSteepLow", sh.colorSteepLow);
    shader.SetVec3("uSteepHigh", sh.colorSteepHigh);
    shader.SetVec3("uSnowColor", sh.colorSnow);

    // Color blending
    shader.SetFloat("uFlatColBlend", sh.flatColBlend);
    shader.SetFloat("uFlatColBlendNoise", sh.flatColBlendNoise);

    // Height range for normalization
    float heightMin = 1.0f - heightScale * HeightRangeMultiplier;
    float heightMax = 1.0f + heightScale * HeightRangeMultiplier;
    shader.SetVec2("uHeightMinMax", glm::vec2(heightMin, heightMax));

    shader.SetFloat("uCoastalDepthRange", sh.coastalDepthRange);
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
        entries.push_back({pe.color, pe.parameter});

    // Reconstruct via internal access (BiomePalette has no public fill ctor — use GetEntries)
    _biomePalette.GetEntries() = std::move(entries);
}

BiomePalette BodyRuntime::LoadBiomePalette() const
{
    EnsurePalette();
    return _biomePalette;
}

} // namespace planets::render
