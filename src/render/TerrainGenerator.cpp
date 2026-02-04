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

std::vector<float> TerrainGenerator::GenerateHeights(
    const std::vector<glm::vec3>& vertices,
    uint32_t seed,
    const EarthTerrainSettings& settings)
{
    // Convert EarthTerrainSettings to ComputeNoiseParams
    ComputeNoiseParams continentParams;
    continentParams.offset = glm::vec3(0.0f);
    continentParams.numLayers = settings.continentOctaves;
    continentParams.scale = settings.continentScale;
    continentParams.persistence = settings.continentPersistence;
    continentParams.lacunarity = settings.continentLacunarity;
    continentParams.multiplier = settings.continentStrength;

    ComputeNoiseParams mountainParams;
    mountainParams.offset = glm::vec3(1000.0f);  // Different offset for variety
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
    maskParams.multiplier = 1.0f;  // Mask output should be normalized

    if (!_computeShader.IsValid())
    {
        return {};
    }

    size_t vertexCount = vertices.size();
    if (vertexCount == 0)
    {
        return {};
    }

    // Pack vertices as flat float array to avoid SSBO alignment issues
    std::vector<float> packedVertices;
    packedVertices.reserve(vertexCount * 3);
    for (const auto& v : vertices)
    {
        packedVertices.push_back(v.x);
        packedVertices.push_back(v.y);
        packedVertices.push_back(v.z);
    }

    // Upload vertices to GPU
    _vertexBuffer.Upload(packedVertices);

    // Allocate output buffer
    _heightBuffer.Allocate(vertexCount);

    // Bind buffers
    _vertexBuffer.Bind(0);
    _heightBuffer.Bind(1);

    // Set uniforms
    _computeShader.Use();
    _computeShader.SetUint("numVertices", static_cast<unsigned int>(vertexCount));
    _computeShader.SetUint("seed", seed);

    // Continent parameters
    _computeShader.SetVec3("continentOffset", continentParams.offset);
    _computeShader.SetInt("continentLayers", continentParams.numLayers);
    _computeShader.SetFloat("continentScale", continentParams.scale);
    _computeShader.SetFloat("continentPersistence", continentParams.persistence);
    _computeShader.SetFloat("continentLacunarity", continentParams.lacunarity);
    _computeShader.SetFloat("continentMultiplier", continentParams.multiplier);

    // Mountain parameters
    _computeShader.SetVec3("mountainOffset", mountainParams.offset);
    _computeShader.SetInt("mountainLayers", mountainParams.numLayers);
    _computeShader.SetFloat("mountainScale", mountainParams.scale);
    _computeShader.SetFloat("mountainPersistence", mountainParams.persistence);
    _computeShader.SetFloat("mountainLacunarity", mountainParams.lacunarity);
    _computeShader.SetFloat("mountainMultiplier", mountainParams.multiplier);
    _computeShader.SetFloat("mountainPower", mountainParams.power);
    _computeShader.SetFloat("mountainGain", mountainParams.gain);
    _computeShader.SetFloat("mountainSmooth", mountainParams.smoothOffset);

    // Mask parameters
    _computeShader.SetVec3("maskOffset", maskParams.offset);
    _computeShader.SetInt("maskLayers", maskParams.numLayers);
    _computeShader.SetFloat("maskScale", maskParams.scale);
    _computeShader.SetFloat("maskPersistence", maskParams.persistence);
    _computeShader.SetFloat("maskLacunarity", maskParams.lacunarity);
    _computeShader.SetFloat("maskMultiplier", maskParams.multiplier);

    // Terrain parameters
    _computeShader.SetFloat("oceanDepthMultiplier", settings.oceanDepthMultiplier);
    _computeShader.SetFloat("oceanFloorDepth", settings.oceanFloorDepth);
    _computeShader.SetFloat("oceanFloorSmoothing", settings.oceanFloorSmoothing);
    _computeShader.SetFloat("mountainBlend", settings.mountainBlend);
    _computeShader.SetFloat("heightScale", settings.heightScale);

    // Dispatch compute shader
    unsigned int groupCount = (static_cast<unsigned int>(vertexCount) + 511) / 512;
    _computeShader.Dispatch(groupCount);

    // Wait for completion
    ComputeShader::WaitForCompletion();

    // Download results
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
    {
        return {};
    }

    // Pack vertices as flat float array to avoid SSBO alignment issues
    std::vector<float> packedVertices;
    packedVertices.reserve(vertexCount * 3);
    for (const auto& v : vertices)
    {
        packedVertices.push_back(v.x);
        packedVertices.push_back(v.y);
        packedVertices.push_back(v.z);
    }

    // Upload vertices to GPU
    _vertexBuffer.Upload(packedVertices);

    // Allocate output buffer
    _heightBuffer.Allocate(vertexCount);

    // Bind buffers
    _vertexBuffer.Bind(0);
    _heightBuffer.Bind(1);

    // Set uniforms
    _computeShader.Use();
    _computeShader.SetUint("numVertices", static_cast<unsigned int>(vertexCount));
    _computeShader.SetUint("seed", seed);

    // Continent parameters
    _computeShader.SetVec3("continentOffset", continentParams.offset);
    _computeShader.SetInt("continentLayers", continentParams.numLayers);
    _computeShader.SetFloat("continentScale", continentParams.scale);
    _computeShader.SetFloat("continentPersistence", continentParams.persistence);
    _computeShader.SetFloat("continentLacunarity", continentParams.lacunarity);
    _computeShader.SetFloat("continentMultiplier", continentParams.multiplier);

    // Mountain parameters
    _computeShader.SetVec3("mountainOffset", mountainParams.offset);
    _computeShader.SetInt("mountainLayers", mountainParams.numLayers);
    _computeShader.SetFloat("mountainScale", mountainParams.scale);
    _computeShader.SetFloat("mountainPersistence", mountainParams.persistence);
    _computeShader.SetFloat("mountainLacunarity", mountainParams.lacunarity);
    _computeShader.SetFloat("mountainMultiplier", mountainParams.multiplier);
    _computeShader.SetFloat("mountainPower", mountainParams.power);
    _computeShader.SetFloat("mountainGain", mountainParams.gain);
    _computeShader.SetFloat("mountainSmooth", mountainParams.smoothOffset);

    // Mask parameters
    _computeShader.SetVec3("maskOffset", maskParams.offset);
    _computeShader.SetInt("maskLayers", maskParams.numLayers);
    _computeShader.SetFloat("maskScale", maskParams.scale);
    _computeShader.SetFloat("maskPersistence", maskParams.persistence);
    _computeShader.SetFloat("maskLacunarity", maskParams.lacunarity);
    _computeShader.SetFloat("maskMultiplier", maskParams.multiplier);

    // Terrain parameters
    _computeShader.SetFloat("oceanDepthMultiplier", oceanDepthMultiplier);
    _computeShader.SetFloat("oceanFloorDepth", oceanFloorDepth);
    _computeShader.SetFloat("oceanFloorSmoothing", oceanFloorSmoothing);
    _computeShader.SetFloat("mountainBlend", mountainBlend);

    // Dispatch compute shader
    unsigned int groupCount = (static_cast<unsigned int>(vertexCount) + 511) / 512;
    _computeShader.Dispatch(groupCount);

    // Wait for completion
    ComputeShader::WaitForCompletion();

    // Download results
    std::vector<float> heights;
    _heightBuffer.Download(heights);

    return heights;
}

} // namespace planets::render
