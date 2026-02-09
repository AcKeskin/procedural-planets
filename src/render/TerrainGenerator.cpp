#include "TerrainGenerator.h"
#include "GpuConstants.h"
#include <iostream>

namespace planets::render
{

namespace
{
    const glm::vec3 ContinentNoiseOffset(0.0f);
    const glm::vec3 MountainNoiseOffset(1000.0f);
    const glm::vec3 MaskNoiseOffset(2000.0f);
    constexpr float DefaultMaskMultiplier = 1.0f;
    constexpr float FallbackHeightScale = 0.04f;
}

TerrainGenerator::TerrainGenerator() = default;
TerrainGenerator::~TerrainGenerator() = default;

bool TerrainGenerator::Initialize(const std::string& shaderPath)
{
    if (!_computeShader.LoadFromFile(shaderPath))
    {
        std::cerr << "[TerrainGenerator] Failed to load compute shader: " << shaderPath << std::endl;
        return false;
    }
    return true;
}

bool TerrainGenerator::Initialize(const std::string& heightShaderPath,
                                  const std::string& shadingShaderPath,
                                  const std::string& erosionShaderPath)
{
    bool success = true;

    if (!_computeShader.LoadFromFile(heightShaderPath))
    {
        std::cerr << "[TerrainGenerator] Failed to load height compute shader: " << heightShaderPath << std::endl;
        success = false;
    }

    if (!_shadingShader.LoadFromFile(shadingShaderPath))
    {
        std::cerr << "[TerrainGenerator] Failed to load shading compute shader: " << shadingShaderPath << std::endl;
        success = false;
    }

    if (!_erosionShader.LoadFromFile(erosionShaderPath))
    {
        std::cerr << "[TerrainGenerator] Failed to load erosion compute shader: " << erosionShaderPath << std::endl;
        success = false;
    }

    return success;
}

void TerrainGenerator::SetHeightUniforms(uint32_t seed, size_t vertexCount, const EarthTerrainSettings& settings)
{
    ComputeNoiseParams continentParams;
    continentParams.offset = ContinentNoiseOffset;
    continentParams.numLayers = settings.continentOctaves;
    continentParams.scale = settings.continentScale;
    continentParams.persistence = settings.continentPersistence;
    continentParams.lacunarity = settings.continentLacunarity;
    continentParams.multiplier = settings.continentStrength;

    ComputeNoiseParams mountainParams;
    mountainParams.offset = MountainNoiseOffset;
    mountainParams.numLayers = settings.mountainOctaves;
    mountainParams.scale = settings.mountainScale;
    mountainParams.persistence = settings.mountainPersistence;
    mountainParams.lacunarity = settings.mountainLacunarity;
    mountainParams.multiplier = settings.mountainStrength;
    mountainParams.power = settings.mountainPower;
    mountainParams.gain = settings.mountainGain;
    mountainParams.smoothOffset = settings.mountainSmoothing;

    ComputeNoiseParams maskParams;
    maskParams.offset = MaskNoiseOffset;
    maskParams.numLayers = settings.maskOctaves;
    maskParams.scale = settings.maskScale;
    maskParams.persistence = settings.maskPersistence;
    maskParams.lacunarity = settings.maskLacunarity;
    maskParams.multiplier = DefaultMaskMultiplier;

    _computeShader.Use();
    _computeShader.SetUint("numVertices", static_cast<unsigned int>(vertexCount));
    _computeShader.SetUint("seed", seed);

    _computeShader.SetVec3("continentOffset", continentParams.offset);
    _computeShader.SetInt("continentLayers", continentParams.numLayers);
    _computeShader.SetFloat("continentScale", continentParams.scale);
    _computeShader.SetFloat("continentPersistence", continentParams.persistence);
    _computeShader.SetFloat("continentLacunarity", continentParams.lacunarity);
    _computeShader.SetFloat("continentMultiplier", continentParams.multiplier);

    _computeShader.SetVec3("mountainOffset", mountainParams.offset);
    _computeShader.SetInt("mountainLayers", mountainParams.numLayers);
    _computeShader.SetFloat("mountainScale", mountainParams.scale);
    _computeShader.SetFloat("mountainPersistence", mountainParams.persistence);
    _computeShader.SetFloat("mountainLacunarity", mountainParams.lacunarity);
    _computeShader.SetFloat("mountainMultiplier", mountainParams.multiplier);
    _computeShader.SetFloat("mountainPower", mountainParams.power);
    _computeShader.SetFloat("mountainGain", mountainParams.gain);
    _computeShader.SetFloat("mountainSmooth", mountainParams.smoothOffset);

    _computeShader.SetVec3("maskOffset", maskParams.offset);
    _computeShader.SetInt("maskLayers", maskParams.numLayers);
    _computeShader.SetFloat("maskScale", maskParams.scale);
    _computeShader.SetFloat("maskPersistence", maskParams.persistence);
    _computeShader.SetFloat("maskLacunarity", maskParams.lacunarity);
    _computeShader.SetFloat("maskMultiplier", maskParams.multiplier);

    _computeShader.SetFloat("oceanDepthMultiplier", settings.oceanDepthMultiplier);
    _computeShader.SetFloat("oceanFloorDepth", settings.oceanFloorDepth);
    _computeShader.SetFloat("oceanFloorSmoothing", settings.oceanFloorSmoothing);
    _computeShader.SetFloat("mountainBlend", settings.mountainBlend);
    _computeShader.SetFloat("heightScale", settings.heightScale);
    _computeShader.SetFloat("continentBaseLevel", settings.continentBaseLevel);

    // Tectonic plate uniforms
    _computeShader.SetInt("useTectonics", settings.useTectonics ? 1 : 0);
    _computeShader.SetInt("numPlates", settings.numPlates);
    _computeShader.SetFloat("continentalFraction", settings.continentalFraction);
    _computeShader.SetFloat("boundaryWidth", settings.boundaryWidth);
    _computeShader.SetFloat("convergentMountainScale", settings.convergentMountainScale);
    _computeShader.SetFloat("divergentRiftDepth", settings.divergentRiftDepth);
    _computeShader.SetFloat("coastlineNoise", settings.coastlineNoise);
    _computeShader.SetFloat("plateElevationNoise", settings.plateElevationNoise);

    _computeShader.SetFloat("detailLowThreshold", settings.detailLowThreshold);
    _computeShader.SetFloat("detailHighThreshold", settings.detailHighThreshold);
    _computeShader.SetFloat("perturbStrengthLow", settings.perturbStrengthLow);
    _computeShader.SetFloat("perturbStrengthHigh", settings.perturbStrengthHigh);
    _computeShader.SetInt("detailOctavesLow", settings.detailOctavesLow);
    _computeShader.SetInt("detailOctavesHigh", settings.detailOctavesHigh);
    _computeShader.SetFloat("detailPersistence", settings.detailPersistence);
    _computeShader.SetFloat("detailLacunarity", settings.detailLacunarity);
    _computeShader.SetFloat("perturbScale", settings.perturbScale);
    _computeShader.SetFloat("globalFrequency", settings.globalFrequency);

    // Ocean floor topology uniforms
    _computeShader.SetInt("useOceanFloor", settings.useOceanFloor ? 1 : 0);
    _computeShader.SetFloat("shelfWidth", settings.shelfWidth);
    _computeShader.SetInt("oceanRidgeOctaves", settings.oceanRidgeOctaves);
    _computeShader.SetFloat("oceanRidgeScale", settings.oceanRidgeScale);
    _computeShader.SetFloat("oceanRidgeStrength", settings.oceanRidgeStrength);
    _computeShader.SetFloat("oceanRidgePower", settings.oceanRidgePower);
    _computeShader.SetFloat("oceanRidgeGain", settings.oceanRidgeGain);
    _computeShader.SetInt("trenchOctaves", settings.trenchOctaves);
    _computeShader.SetFloat("trenchScale", settings.trenchScale);
    _computeShader.SetFloat("trenchDepth", settings.trenchDepth);
    _computeShader.SetInt("abyssalOctaves", settings.abyssalOctaves);
    _computeShader.SetFloat("abyssalScale", settings.abyssalScale);
    _computeShader.SetFloat("abyssalStrength", settings.abyssalStrength);
}

void TerrainGenerator::SetShadingUniforms(uint32_t seed,
                                          size_t vertexCount,
                                          const EarthShadingSettings& settings,
                                          float heightScale)
{
    _shadingShader.Use();
    _shadingShader.SetUint("numVertices", static_cast<unsigned int>(vertexCount));
    _shadingShader.SetUint("seed", seed);

    _shadingShader.SetFloat("largeNoiseScale", settings.largeNoiseScale);
    _shadingShader.SetInt("largeNoiseOctaves", settings.largeNoiseOctaves);
    _shadingShader.SetFloat("detailNoiseScale", settings.detailNoiseScale);
    _shadingShader.SetFloat("smallNoiseScale", settings.smallNoiseScale);
    _shadingShader.SetInt("smallNoiseOctaves", settings.smallNoiseOctaves);
    _shadingShader.SetFloat("warpStrength", settings.warpStrength);

    // Climate model uniforms
    _shadingShader.SetInt("useClimateModel", settings.useClimateModel ? 1 : 0);
    _shadingShader.SetFloat("temperatureLapseRate", settings.temperatureLapseRate);
    _shadingShader.SetFloat("temperatureExponent", settings.temperatureExponent);
    _shadingShader.SetFloat("moistureNoiseScale", settings.moistureNoiseScale);
    _shadingShader.SetFloat("moistureNoiseStrength", settings.moistureNoiseStrength);
    _shadingShader.SetFloat("hadleyIntensity", settings.hadleyIntensity);
    _shadingShader.SetFloat("continentalityStrength", settings.continentalityStrength);
    _shadingShader.SetFloat("heightScale", heightScale);
}

std::vector<float> TerrainGenerator::GenerateHeights(const std::vector<glm::vec3>& vertices,
                                                     uint32_t seed,
                                                     const EarthTerrainSettings& settings)
{
    if (!_computeShader.IsValid())
        return {};

    size_t vertexCount = vertices.size();
    if (vertexCount == 0)
        return {};

    std::vector<float> packedVertices;
    packedVertices.reserve(vertexCount * 3);
    for (const auto& v : vertices)
    {
        packedVertices.push_back(v.x);
        packedVertices.push_back(v.y);
        packedVertices.push_back(v.z);
    }

    _vertexBuffer.Upload(packedVertices);
    _heightBuffer.Allocate(vertexCount);
    _vertexBuffer.Bind(0);
    _heightBuffer.Bind(1);

    SetHeightUniforms(seed, vertexCount, settings);

    unsigned int groupCount = (static_cast<unsigned int>(vertexCount) + HeightWorkgroupSize - 1) / HeightWorkgroupSize;
    _computeShader.Dispatch(groupCount);
    ComputeShader::WaitForCompletion();

    std::vector<float> heights;
    _heightBuffer.Download(heights);
    return heights;
}

std::vector<float> TerrainGenerator::GenerateHeights(const std::vector<glm::vec3>& vertices,
                                                     uint32_t seed,
                                                     const ComputeNoiseParams& continentParams,
                                                     const ComputeNoiseParams& mountainParams,
                                                     const ComputeNoiseParams& maskParams,
                                                     float oceanDepthMultiplier,
                                                     float oceanFloorDepth,
                                                     float oceanFloorSmoothing,
                                                     float mountainBlend)
{
    if (!_computeShader.IsValid())
    {
        std::cerr << "[TerrainGenerator] Compute shader not initialized" << std::endl;
        return {};
    }

    size_t vertexCount = vertices.size();
    if (vertexCount == 0)
        return {};

    std::vector<float> packedVertices;
    packedVertices.reserve(vertexCount * 3);
    for (const auto& v : vertices)
    {
        packedVertices.push_back(v.x);
        packedVertices.push_back(v.y);
        packedVertices.push_back(v.z);
    }

    _vertexBuffer.Upload(packedVertices);
    _heightBuffer.Allocate(vertexCount);
    _vertexBuffer.Bind(0);
    _heightBuffer.Bind(1);

    _computeShader.Use();
    _computeShader.SetUint("numVertices", static_cast<unsigned int>(vertexCount));
    _computeShader.SetUint("seed", seed);

    _computeShader.SetVec3("continentOffset", continentParams.offset);
    _computeShader.SetInt("continentLayers", continentParams.numLayers);
    _computeShader.SetFloat("continentScale", continentParams.scale);
    _computeShader.SetFloat("continentPersistence", continentParams.persistence);
    _computeShader.SetFloat("continentLacunarity", continentParams.lacunarity);
    _computeShader.SetFloat("continentMultiplier", continentParams.multiplier);

    _computeShader.SetVec3("mountainOffset", mountainParams.offset);
    _computeShader.SetInt("mountainLayers", mountainParams.numLayers);
    _computeShader.SetFloat("mountainScale", mountainParams.scale);
    _computeShader.SetFloat("mountainPersistence", mountainParams.persistence);
    _computeShader.SetFloat("mountainLacunarity", mountainParams.lacunarity);
    _computeShader.SetFloat("mountainMultiplier", mountainParams.multiplier);
    _computeShader.SetFloat("mountainPower", mountainParams.power);
    _computeShader.SetFloat("mountainGain", mountainParams.gain);
    _computeShader.SetFloat("mountainSmooth", mountainParams.smoothOffset);

    _computeShader.SetVec3("maskOffset", maskParams.offset);
    _computeShader.SetInt("maskLayers", maskParams.numLayers);
    _computeShader.SetFloat("maskScale", maskParams.scale);
    _computeShader.SetFloat("maskPersistence", maskParams.persistence);
    _computeShader.SetFloat("maskLacunarity", maskParams.lacunarity);
    _computeShader.SetFloat("maskMultiplier", maskParams.multiplier);

    _computeShader.SetFloat("oceanDepthMultiplier", oceanDepthMultiplier);
    _computeShader.SetFloat("oceanFloorDepth", oceanFloorDepth);
    _computeShader.SetFloat("oceanFloorSmoothing", oceanFloorSmoothing);
    _computeShader.SetFloat("mountainBlend", mountainBlend);

    unsigned int groupCount = (static_cast<unsigned int>(vertexCount) + 511) / 512;
    _computeShader.Dispatch(groupCount);
    ComputeShader::WaitForCompletion();

    std::vector<float> heights;
    _heightBuffer.Download(heights);
    return heights;
}

std::vector<glm::vec4> TerrainGenerator::GenerateShadingData(const std::vector<glm::vec3>& vertices,
                                                             uint32_t seed,
                                                             const EarthShadingSettings& settings)
{
    if (!_shadingShader.IsValid())
    {
        std::cerr << "[TerrainGenerator] Shading compute shader not initialized" << std::endl;
        return {};
    }

    size_t vertexCount = vertices.size();
    if (vertexCount == 0)
        return {};

    std::vector<float> packedVertices;
    packedVertices.reserve(vertexCount * 3);
    for (const auto& v : vertices)
    {
        packedVertices.push_back(v.x);
        packedVertices.push_back(v.y);
        packedVertices.push_back(v.z);
    }

    _vertexBuffer.Upload(packedVertices);
    _shadingBuffer.Allocate(vertexCount);
    _vertexBuffer.Bind(0);
    _shadingBuffer.Bind(1);
    // Bind sea-level height buffer for climate model (elevation data unavailable in sync path)
    GpuBuffer<float> zeroHeightBuffer;
    std::vector<float> seaLevelHeights(vertexCount, 1.0f);
    zeroHeightBuffer.Upload(seaLevelHeights);
    zeroHeightBuffer.Bind(2);

    SetShadingUniforms(seed, vertexCount, settings, FallbackHeightScale);

    unsigned int groupCount = (static_cast<unsigned int>(vertexCount) + ShadingWorkgroupSize - 1) / ShadingWorkgroupSize;
    _shadingShader.Dispatch(groupCount);
    ComputeShader::WaitForCompletion();

    std::vector<glm::vec4> shadingData;
    _shadingBuffer.Download(shadingData);
    return shadingData;
}

void TerrainGenerator::DispatchHeightsAsync(GpuBuffer<float>& vertexBuffer,
                                            GpuBuffer<float>& heightBuffer,
                                            size_t vertexCount,
                                            uint32_t seed,
                                            const EarthTerrainSettings& settings)
{
    vertexBuffer.Bind(0);
    heightBuffer.Bind(1);

    SetHeightUniforms(seed, vertexCount, settings);

    unsigned int groupCount = (static_cast<unsigned int>(vertexCount) + HeightWorkgroupSize - 1) / HeightWorkgroupSize;
    _computeShader.Dispatch(groupCount);
}

void TerrainGenerator::DispatchShadingAsync(GpuBuffer<float>& vertexBuffer,
                                            GpuBuffer<glm::vec4>& shadingBuffer,
                                            GpuBuffer<float>& heightBuffer,
                                            size_t vertexCount,
                                            uint32_t seed,
                                            const EarthShadingSettings& settings,
                                            float heightScale)
{
    vertexBuffer.Bind(0);
    shadingBuffer.Bind(1);
    heightBuffer.Bind(2);

    SetShadingUniforms(seed, vertexCount, settings, heightScale);

    unsigned int groupCount = (static_cast<unsigned int>(vertexCount) + ShadingWorkgroupSize - 1) / ShadingWorkgroupSize;
    _shadingShader.Dispatch(groupCount);
}

void TerrainGenerator::SetErosionUniforms(size_t vertexCount, int gridResolution, const EarthTerrainSettings& settings)
{
    _erosionShader.Use();
    _erosionShader.SetUint("numVertices", static_cast<unsigned int>(vertexCount));
    _erosionShader.SetInt("gridResolution", gridResolution);
    _erosionShader.SetFloat("thermalRate", settings.thermalErosionRate);
    _erosionShader.SetFloat("thermalThreshold", settings.thermalThreshold);
    _erosionShader.SetFloat("hydraulicRate", settings.hydraulicErosionRate);
    _erosionShader.SetFloat("depositionRate", settings.depositionRate);
    _erosionShader.SetFloat("evaporationRate", settings.evaporationRate);
}

void TerrainGenerator::DispatchErosionAsync(GpuBuffer<float>& inputBuffer,
                                            GpuBuffer<float>& outputBuffer,
                                            size_t vertexCount,
                                            int gridResolution,
                                            const EarthTerrainSettings& settings)
{
    inputBuffer.Bind(0);
    outputBuffer.Bind(1);
    SetErosionUniforms(vertexCount, gridResolution, settings);

    unsigned int groupCount = (static_cast<unsigned int>(vertexCount) + ErosionWorkgroupSize - 1) / ErosionWorkgroupSize;
    _erosionShader.Dispatch(groupCount);
}

} // namespace planets::render
