#include "TerrainGenerator.h"
#include <iostream>

namespace planets::render {

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

bool TerrainGenerator::Initialize(const std::string& heightShaderPath, const std::string& shadingShaderPath)
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

    return success;
}

void TerrainGenerator::SetHeightUniforms(uint32_t seed, size_t vertexCount, const EarthTerrainSettings& settings)
{
    ComputeNoiseParams continentParams;
    continentParams.offset = glm::vec3(0.0f);
    continentParams.numLayers = settings.continentOctaves;
    continentParams.scale = settings.continentScale;
    continentParams.persistence = settings.continentPersistence;
    continentParams.lacunarity = settings.continentLacunarity;
    continentParams.multiplier = settings.continentStrength;

    ComputeNoiseParams mountainParams;
    mountainParams.offset = glm::vec3(1000.0f);
    mountainParams.numLayers = settings.mountainOctaves;
    mountainParams.scale = settings.mountainScale;
    mountainParams.persistence = settings.mountainPersistence;
    mountainParams.lacunarity = settings.mountainLacunarity;
    mountainParams.multiplier = settings.mountainStrength;
    mountainParams.power = settings.mountainPower;
    mountainParams.gain = settings.mountainGain;
    mountainParams.smoothOffset = settings.mountainSmoothing;

    ComputeNoiseParams maskParams;
    maskParams.offset = glm::vec3(2000.0f);
    maskParams.numLayers = settings.maskOctaves;
    maskParams.scale = settings.maskScale;
    maskParams.persistence = settings.maskPersistence;
    maskParams.lacunarity = settings.maskLacunarity;
    maskParams.multiplier = 1.0f;

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

    _computeShader.SetFloat("perturbStrength", settings.perturbStrength);
    _computeShader.SetFloat("perturbScale", settings.perturbScale);
    _computeShader.SetFloat("globalFrequency", settings.globalFrequency);
}

void TerrainGenerator::SetShadingUniforms(uint32_t seed, size_t vertexCount, const EarthShadingSettings& settings)
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
}

std::vector<float> TerrainGenerator::GenerateHeights(
    const std::vector<glm::vec3>& vertices,
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

    unsigned int groupCount = (static_cast<unsigned int>(vertexCount) + 511) / 512;
    _computeShader.Dispatch(groupCount);
    ComputeShader::WaitForCompletion();

    std::vector<float> heights;
    _heightBuffer.Download(heights);
    return heights;
}

std::vector<float> TerrainGenerator::GenerateHeights(
    const std::vector<glm::vec3>& vertices,
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

std::vector<glm::vec4> TerrainGenerator::GenerateShadingData(
    const std::vector<glm::vec3>& vertices,
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

    SetShadingUniforms(seed, vertexCount, settings);

    unsigned int groupCount = (static_cast<unsigned int>(vertexCount) + 255) / 256;
    _shadingShader.Dispatch(groupCount);
    ComputeShader::WaitForCompletion();

    std::vector<glm::vec4> shadingData;
    _shadingBuffer.Download(shadingData);
    return shadingData;
}

void TerrainGenerator::DispatchHeightsAsync(
    GpuBuffer<float>& vertexBuffer,
    GpuBuffer<float>& heightBuffer,
    size_t vertexCount,
    uint32_t seed,
    const EarthTerrainSettings& settings)
{
    vertexBuffer.Bind(0);
    heightBuffer.Bind(1);

    SetHeightUniforms(seed, vertexCount, settings);

    unsigned int groupCount = (static_cast<unsigned int>(vertexCount) + 511) / 512;
    _computeShader.Dispatch(groupCount);
}

void TerrainGenerator::DispatchShadingAsync(
    GpuBuffer<float>& vertexBuffer,
    GpuBuffer<glm::vec4>& shadingBuffer,
    size_t vertexCount,
    uint32_t seed,
    const EarthShadingSettings& settings)
{
    vertexBuffer.Bind(0);
    shadingBuffer.Bind(1);

    SetShadingUniforms(seed, vertexCount, settings);

    unsigned int groupCount = (static_cast<unsigned int>(vertexCount) + 255) / 256;
    _shadingShader.Dispatch(groupCount);
}

} // namespace planets::render
