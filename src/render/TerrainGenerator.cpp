#include "TerrainGenerator.h"
#include "GpuConstants.h"
#include <iostream>

namespace planets::render
{

TerrainGenerator::TerrainGenerator() = default;

TerrainGenerator::~TerrainGenerator()
{
    if (_pgContext)
        pg_context_destroy(_pgContext);
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

bool TerrainGenerator::InitializeLibrary()
{
    if (_pgContext)
        return true; // Already initialized

    PgContextDesc desc{};
    desc.use_existing_context = 1; // Reuse app's GL context
    _pgContext = pg_context_create(&desc);

    if (!_pgContext)
    {
        std::cerr << "[TerrainGenerator] Failed to initialize libplanetgen context" << std::endl;
        return false;
    }

    PgError err = pg_context_get_error(_pgContext);
    if (err != PG_SUCCESS)
    {
        std::cerr << "[TerrainGenerator] libplanetgen context created with errors (code " << err << ")" << std::endl;
    }

    std::cout << "[TerrainGenerator] libplanetgen context initialized (embedded shaders)" << std::endl;
    return true;
}

// ============================================================================
// Async dispatch — the live LOD path, driven by BodyRuntime
// ============================================================================

void TerrainGenerator::DispatchHeightsAsync(GpuBuffer<float>& vertexBuffer,
                                            GpuBuffer<float>& heightBuffer,
                                            GpuBuffer<float>& normalBuffer,
                                            size_t vertexCount,
                                            const BodyRuntime& body,
                                            uint32_t seed)
{
    vertexBuffer.Bind(0);
    heightBuffer.Bind(1);
    normalBuffer.Bind(2);

    body.SetShapeUniforms(_computeShader, seed, static_cast<uint32_t>(vertexCount));

    unsigned int groupCount = (static_cast<unsigned int>(vertexCount) + HeightWorkgroupSize - 1) / HeightWorkgroupSize;
    _computeShader.Dispatch(groupCount);
}

void TerrainGenerator::DispatchShadingAsync(GpuBuffer<float>& vertexBuffer,
                                            GpuBuffer<glm::vec4>& shadingBuffer,
                                            GpuBuffer<float>& heightBuffer,
                                            size_t vertexCount,
                                            const BodyRuntime& body,
                                            uint32_t seed)
{
    vertexBuffer.Bind(0);
    shadingBuffer.Bind(1);
    heightBuffer.Bind(2);

    body.SetShadingUniforms(_shadingShader, seed, static_cast<uint32_t>(vertexCount));

    unsigned int groupCount =
        (static_cast<unsigned int>(vertexCount) + ShadingWorkgroupSize - 1) / ShadingWorkgroupSize;
    _shadingShader.Dispatch(groupCount);
}

void TerrainGenerator::DispatchErosionAsync(GpuBuffer<float>& inputBuffer,
                                            GpuBuffer<float>& outputBuffer,
                                            size_t vertexCount,
                                            int gridResolution,
                                            const BodyRuntime& body)
{
    inputBuffer.Bind(0);
    outputBuffer.Bind(1);

    body.SetErosionUniforms(_erosionShader, static_cast<uint32_t>(vertexCount), gridResolution);

    unsigned int groupCount =
        (static_cast<unsigned int>(vertexCount) + ErosionWorkgroupSize - 1) / ErosionWorkgroupSize;
    _erosionShader.Dispatch(groupCount);
}

} // namespace planets::render
