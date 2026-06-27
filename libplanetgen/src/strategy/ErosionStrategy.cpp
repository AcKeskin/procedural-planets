#include "ErosionStrategy.h"
#include "../backend/ParamBlocks.h"
#include "../backend/ParamMappers.h"

namespace planetgen
{

namespace
{
constexpr uint32_t ErosionWorkgroupSize = 256;
constexpr uint32_t ParamBinding = 3;

uint32_t GroupCount(uint32_t count, uint32_t wg) { return (count + wg - 1) / wg; }
} // namespace

GpuBufferHandle ErosionStrategy::Run(const StrategyContext& sc,
                                     const PgErosionDesc& desc,
                                     GpuBufferHandle heights)
{
    auto& gpu = sc.backend;

    const uint32_t program = sc.registry.Resolve(BuiltinProgram::Erosion);
    if (!program || desc.iterations <= 0)
        return heights; // nothing to do — pass the input through unchanged

    const size_t heightBytes = sc.vertexCount * sizeof(float);

    // The caller owns `heights`; we ping-pong between it and one scratch buffer.
    GpuBufferHandle in  = heights;
    GpuBufferHandle out = gpu.CreateBuffer(heightBytes);

    gpu.BindShader(program);

    const auto params = MakeErosionParams(desc, sc.vertexCount);
    const GpuBufferHandle ubo = gpu.CreateParamBuffer(sizeof(params));

    for (int i = 0; i < desc.iterations; ++i)
    {
        gpu.BindBuffer(in,  0);
        gpu.BindBuffer(out, 1);
        gpu.SetParams(ubo, &params, sizeof(params), ParamBinding);
        gpu.Dispatch(GroupCount(sc.vertexCount, ErosionWorkgroupSize));
        gpu.Barrier();

        std::swap(in, out);
    }

    gpu.DestroyParamBuffer(ubo);

    // After the swaps, `in` holds the latest result. `out` is the stale partner;
    // destroy it only if it's the scratch buffer (never the caller's `heights`).
    if (out != heights)
        gpu.DestroyBuffer(out);

    return in;
}

} // namespace planetgen
