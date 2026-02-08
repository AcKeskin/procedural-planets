#include "TerrainGenerator.h"
#include "../../render/GpuConstants.h"
#include <iostream>

namespace planets::core
{

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
    {
        return {};
    }

    // Upload vertices to GPU
    _vertexBuffer.Upload(vertices);

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
    unsigned int groupCount = (static_cast<unsigned int>(vertexCount) + render::HeightWorkgroupSize - 1) / render::HeightWorkgroupSize;
    _computeShader.Dispatch(groupCount);

    // Wait for completion
    render::ComputeShader::MemoryBarrier();

    // Download results
    std::vector<float> heights;
    _heightBuffer.Download(heights);

    return heights;
}

} // namespace planets::core
