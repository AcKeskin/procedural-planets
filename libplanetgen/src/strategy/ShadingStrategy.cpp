#include "ShadingStrategy.h"
#include "StrategyCommon.h"
#include "../backend/ParamBlocks.h"
#include "../backend/ParamMappers.h"

namespace planetgen
{

namespace
{
constexpr uint32_t ShadingWorkgroupSize = 256;
} // namespace

GpuBufferHandle ShadingStrategy::Run(const StrategyContext& sc,
                                     const PgShadingDesc& desc,
                                     GpuBufferHandle vertices,
                                     GpuBufferHandle heights)
{
    auto& gpu = sc.backend;

    // Earth/generic choice lives here, not in the caller.
    BuiltinProgram programId = desc.use_climate_model ? BuiltinProgram::ShadingEarth
                                                      : BuiltinProgram::ShadingGeneric;
    uint32_t program = sc.registry.Resolve(programId);
    if (!program)
    {
        // Fall back to whichever is available, preserving prior behaviour.
        programId = BuiltinProgram::ShadingEarth;
        program = sc.registry.Resolve(programId);
        if (!program)
        {
            programId = BuiltinProgram::ShadingGeneric;
            program = sc.registry.Resolve(programId);
        }
        if (!program)
            return 0;
    }

    const size_t shadingBytes = sc.vertexCount * 4 * sizeof(float);
    // Per-patch path passes an app-owned shading buffer; otherwise allocate our own.
    const GpuBufferHandle shadingBuf = sc.outShading ? sc.outShading : gpu.CreateBuffer(shadingBytes);

    gpu.BindShader(program);
    gpu.BindBuffer(vertices,   0);
    gpu.BindBuffer(shadingBuf, 1);
    gpu.BindBuffer(heights,    2);

    GpuBufferHandle ubo = 0;
    if (programId == BuiltinProgram::ShadingEarth)
    {
        const auto p = MakeShadingEarthParams(desc, sc.seed, sc.vertexCount);
        ubo = gpu.CreateParamBuffer(sizeof(p));
        gpu.SetParams(ubo, &p, sizeof(p), kParamBinding);
    }
    else
    {
        const auto p = MakeShadingGenericParams(desc, sc.seed, sc.vertexCount);
        ubo = gpu.CreateParamBuffer(sizeof(p));
        gpu.SetParams(ubo, &p, sizeof(p), kParamBinding);
    }

    gpu.Dispatch(GroupCount(sc.vertexCount, ShadingWorkgroupSize));
    gpu.Barrier();

    gpu.DestroyParamBuffer(ubo);

    return shadingBuf;
}

} // namespace planetgen
