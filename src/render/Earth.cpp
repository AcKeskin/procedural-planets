#include "Earth.h"
#include "ComputeShader.h"
#include "Shader.h"
#include <glm/glm.hpp>

namespace planets::render
{

namespace
{
// Noise domain offsets to decorrelate noise layers
const glm::vec3 ContinentNoiseOffset(0.0f);
const glm::vec3 MountainNoiseOffset(1000.0f);
const glm::vec3 MaskNoiseOffset(2000.0f);
constexpr float DefaultMaskMultiplier = 1.0f;

// Shader and data paths
constexpr const char* HeightShaderPath = "shaders/compute/height_earth.comp";
constexpr const char* ShadingShaderPath = "shaders/compute/shading_earth.comp";
constexpr const char* VertexShaderPath = "shaders/earth/planet_earth.vert";
constexpr const char* FragmentShaderPath = "shaders/earth/planet_earth.frag";
constexpr const char* PalettePath = "data/palettes/earth.json";
constexpr const char* ErosionShaderPath = "shaders/compute/erosion_earth.comp";
constexpr float HeightRangeMultiplier = 1.5f;
} // namespace

Earth::Earth()
{
    _radius = 10.0f;
    _seaLevel = 0.995f;
    _atmosphereHeight = 0.08f;
}

std::string Earth::GetHeightShaderPath() const
{
    return HeightShaderPath;
}

std::string Earth::GetShadingShaderPath() const
{
    return ShadingShaderPath;
}

std::string Earth::GetVertexShaderPath() const
{
    return VertexShaderPath;
}

std::string Earth::GetFragmentShaderPath() const
{
    return FragmentShaderPath;
}

BiomePalette Earth::LoadBiomePalette() const
{
    return BiomePalette::LoadFromJson(PalettePath);
}

void Earth::SetShapeUniforms(ComputeShader& shader, uint32_t seed) const
{
    shader.Use();
    shader.SetUint("seed", seed);

    // Continental noise configuration
    shader.SetVec3("continentOffset", ContinentNoiseOffset);
    shader.SetInt("continentLayers", _terrainSettings.continentOctaves);
    shader.SetFloat("continentScale", _terrainSettings.continentScale);
    shader.SetFloat("continentPersistence", _terrainSettings.continentPersistence);
    shader.SetFloat("continentLacunarity", _terrainSettings.continentLacunarity);
    shader.SetFloat("continentMultiplier", _terrainSettings.continentStrength);

    // Mountain ridge noise configuration
    shader.SetVec3("mountainOffset", MountainNoiseOffset);
    shader.SetInt("mountainLayers", _terrainSettings.mountainOctaves);
    shader.SetFloat("mountainScale", _terrainSettings.mountainScale);
    shader.SetFloat("mountainPersistence", _terrainSettings.mountainPersistence);
    shader.SetFloat("mountainLacunarity", _terrainSettings.mountainLacunarity);
    shader.SetFloat("mountainMultiplier", _terrainSettings.mountainStrength);
    shader.SetFloat("mountainPower", _terrainSettings.mountainPower);
    shader.SetFloat("mountainGain", _terrainSettings.mountainGain);
    shader.SetFloat("mountainSmooth", _terrainSettings.mountainSmoothing);

    // Mask noise configuration
    shader.SetVec3("maskOffset", MaskNoiseOffset);
    shader.SetInt("maskLayers", _terrainSettings.maskOctaves);
    shader.SetFloat("maskScale", _terrainSettings.maskScale);
    shader.SetFloat("maskPersistence", _terrainSettings.maskPersistence);
    shader.SetFloat("maskLacunarity", _terrainSettings.maskLacunarity);
    shader.SetFloat("maskMultiplier", DefaultMaskMultiplier);

    // Ocean and terrain blending
    shader.SetFloat("oceanDepthMultiplier", _terrainSettings.oceanDepthMultiplier);
    shader.SetFloat("oceanFloorDepth", _terrainSettings.oceanFloorDepth);
    shader.SetFloat("oceanFloorSmoothing", _terrainSettings.oceanFloorSmoothing);
    shader.SetFloat("mountainBlend", _terrainSettings.mountainBlend);
    shader.SetFloat("heightScale", _terrainSettings.heightScale);
    shader.SetFloat("continentBaseLevel", _terrainSettings.continentBaseLevel);

    // Tectonic plate uniforms
    shader.SetInt("useTectonics", _terrainSettings.useTectonics ? 1 : 0);
    shader.SetInt("numPlates", _terrainSettings.numPlates);
    shader.SetFloat("continentalFraction", _terrainSettings.continentalFraction);
    shader.SetFloat("boundaryWidth", _terrainSettings.boundaryWidth);
    shader.SetFloat("convergentMountainScale", _terrainSettings.convergentMountainScale);
    shader.SetFloat("divergentRiftDepth", _terrainSettings.divergentRiftDepth);
    shader.SetFloat("coastlineNoise", _terrainSettings.coastlineNoise);
    shader.SetFloat("plateElevationNoise", _terrainSettings.plateElevationNoise);

    // Height-dependent detail noise
    shader.SetFloat("detailLowThreshold", _terrainSettings.detailLowThreshold);
    shader.SetFloat("detailHighThreshold", _terrainSettings.detailHighThreshold);
    shader.SetFloat("perturbStrengthLow", _terrainSettings.perturbStrengthLow);
    shader.SetFloat("perturbStrengthHigh", _terrainSettings.perturbStrengthHigh);
    shader.SetInt("detailOctavesLow", _terrainSettings.detailOctavesLow);
    shader.SetInt("detailOctavesHigh", _terrainSettings.detailOctavesHigh);
    shader.SetFloat("detailPersistence", _terrainSettings.detailPersistence);
    shader.SetFloat("detailLacunarity", _terrainSettings.detailLacunarity);
    shader.SetFloat("perturbScale", _terrainSettings.perturbScale);
    shader.SetFloat("globalFrequency", _terrainSettings.globalFrequency);

    // Ocean floor topology
    shader.SetInt("useOceanFloor", _terrainSettings.useOceanFloor ? 1 : 0);
    shader.SetFloat("shelfWidth", _terrainSettings.shelfWidth);
    shader.SetInt("oceanRidgeOctaves", _terrainSettings.oceanRidgeOctaves);
    shader.SetFloat("oceanRidgeScale", _terrainSettings.oceanRidgeScale);
    shader.SetFloat("oceanRidgeStrength", _terrainSettings.oceanRidgeStrength);
    shader.SetFloat("oceanRidgePower", _terrainSettings.oceanRidgePower);
    shader.SetFloat("oceanRidgeGain", _terrainSettings.oceanRidgeGain);
    shader.SetInt("trenchOctaves", _terrainSettings.trenchOctaves);
    shader.SetFloat("trenchScale", _terrainSettings.trenchScale);
    shader.SetFloat("trenchDepth", _terrainSettings.trenchDepth);
    shader.SetInt("abyssalOctaves", _terrainSettings.abyssalOctaves);
    shader.SetFloat("abyssalScale", _terrainSettings.abyssalScale);
    shader.SetFloat("abyssalStrength", _terrainSettings.abyssalStrength);

    // Analytical normal computation
    shader.SetFloat("normalEpsilon", _terrainSettings.normalEpsilon);
}

void Earth::SetShadingUniforms(ComputeShader& shader, uint32_t seed) const
{
    shader.Use();
    shader.SetUint("seed", seed);

    shader.SetFloat("largeNoiseScale", _shadingSettings.largeNoiseScale);
    shader.SetInt("largeNoiseOctaves", _shadingSettings.largeNoiseOctaves);
    shader.SetFloat("detailNoiseScale", _shadingSettings.detailNoiseScale);
    shader.SetFloat("smallNoiseScale", _shadingSettings.smallNoiseScale);
    shader.SetInt("smallNoiseOctaves", _shadingSettings.smallNoiseOctaves);
    shader.SetFloat("warpStrength", _shadingSettings.warpStrength);

    // Climate model uniforms
    shader.SetInt("useClimateModel", _shadingSettings.useClimateModel ? 1 : 0);
    shader.SetFloat("temperatureLapseRate", _shadingSettings.temperatureLapseRate);
    shader.SetFloat("temperatureExponent", _shadingSettings.temperatureExponent);
    shader.SetFloat("moistureNoiseScale", _shadingSettings.moistureNoiseScale);
    shader.SetFloat("moistureNoiseStrength", _shadingSettings.moistureNoiseStrength);
    shader.SetFloat("hadleyIntensity", _shadingSettings.hadleyIntensity);
    shader.SetFloat("continentalityStrength", _shadingSettings.continentalityStrength);
    shader.SetFloat("heightScale", _terrainSettings.heightScale);
}

std::string Earth::GetErosionShaderPath() const
{
    return ErosionShaderPath;
}

void Earth::SetRenderUniforms(Shader& shader) const
{
    // Biome settings
    shader.SetInt("uUseBiomes", _biomeSettings.enabled ? 1 : 0);
    shader.SetFloat("uSteepnessThreshold", _biomeSettings.steepnessThreshold);
    shader.SetFloat("uFlatToSteepBlend", _biomeSettings.flatToSteepBlend);
    shader.SetFloat("uSnowLatitude", _biomeSettings.snowLatitude);
    shader.SetFloat("uSnowBlend", _biomeSettings.snowBlend);
    shader.SetFloat("uSnowLine", _biomeSettings.snowLine);
    shader.SetFloat("uShoreHeight", _biomeSettings.shoreHeight);
    shader.SetInt("uUseClimateModel", _shadingSettings.useClimateModel ? 1 : 0);

    // Earth color palette
    shader.SetVec3("uShoreLow", _colors.shoreLow);
    shader.SetVec3("uShoreHigh", _colors.shoreHigh);
    shader.SetVec3("uFlatLowA", _colors.flatLowA);
    shader.SetVec3("uFlatHighA", _colors.flatHighA);
    shader.SetVec3("uFlatLowB", _colors.flatLowB);
    shader.SetVec3("uFlatHighB", _colors.flatHighB);
    shader.SetVec3("uSteepLow", _colors.steepLow);
    shader.SetVec3("uSteepHigh", _colors.steepHigh);
    shader.SetVec3("uSnowColor", _colors.snow);

    // Shading blend parameters
    shader.SetFloat("uFlatColBlend", _shadingSettings.flatColBlend);
    shader.SetFloat("uFlatColBlendNoise", _shadingSettings.flatColBlendNoise);

    // Height range
    float heightMin = 1.0f - _terrainSettings.heightScale * HeightRangeMultiplier;
    float heightMax = 1.0f + _terrainSettings.heightScale * HeightRangeMultiplier;
    shader.SetVec2("uHeightMinMax", glm::vec2(heightMin, heightMax));

    // Earth-specific visual quality
    shader.SetFloat("uCoastalDepthRange", _biomeSettings.coastalDepthRange);
}

bool Earth::SupportsErosion() const
{
    return _terrainSettings.enableErosion;
}

void Earth::SetErosionUniforms(ComputeShader& shader, size_t vertexCount, int gridResolution) const
{
    shader.Use();
    shader.SetUint("numVertices", static_cast<unsigned int>(vertexCount));
    shader.SetInt("gridResolution", gridResolution);
    shader.SetFloat("thermalRate", _terrainSettings.thermalErosionRate);
    shader.SetFloat("thermalThreshold", _terrainSettings.thermalThreshold);
    shader.SetFloat("hydraulicRate", _terrainSettings.hydraulicErosionRate);
    shader.SetFloat("depositionRate", _terrainSettings.depositionRate);
    shader.SetFloat("evaporationRate", _terrainSettings.evaporationRate);
}

int Earth::GetErosionIterations() const
{
    return _terrainSettings.erosionIterations;
}

} // namespace planets::render
