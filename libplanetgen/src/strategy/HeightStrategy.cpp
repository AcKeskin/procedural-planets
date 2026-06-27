#include "HeightStrategy.h"
#include "StrategyCommon.h"
#include "../backend/ParamBlocks.h"
#include "../backend/ParamMappers.h"

namespace planetgen
{

namespace
{
constexpr uint32_t HeightWorkgroupSize = 512;
} // namespace

HeightBuffers HeightStrategy::Run(const StrategyContext& sc,
                                  const PgBodyDesc& desc,
                                  GpuBufferHandle vertices)
{
    auto& gpu = sc.backend;

    const uint32_t program = sc.registry.Resolve(BuiltinProgram::Height);
    if (!program)
        return {};

    const size_t heightBytes = sc.vertexCount * sizeof(float);
    const size_t normalBytes = sc.vertexCount * 3 * sizeof(float);

    const GpuBufferHandle heightBuf = gpu.CreateBuffer(heightBytes);
    const GpuBufferHandle normalBuf = gpu.CreateBuffer(normalBytes);

    gpu.BindShader(program);
    gpu.BindBuffer(vertices,  0);
    gpu.BindBuffer(heightBuf, 1);
    gpu.BindBuffer(normalBuf, 2);

    const auto params = MakeHeightParams(desc, sc.seed, sc.vertexCount);
    const GpuBufferHandle ubo = gpu.CreateParamBuffer(sizeof(params));
    gpu.SetParams(ubo, &params, sizeof(params), kParamBinding);

    gpu.Dispatch(GroupCount(sc.vertexCount, HeightWorkgroupSize));
    gpu.Barrier();

    gpu.DestroyParamBuffer(ubo);

    return { heightBuf, normalBuf };
}

} // namespace planetgen
