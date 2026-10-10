#include "HeightStrategy.h"
#include "StrategyCommon.h"
#include "../backend/ParamBlocks.h"
#include "../backend/ParamMappers.h"
#include "MacroWorldPass.h"

namespace planetgen
{

namespace
{
constexpr uint32_t HeightWorkgroupSize = 512;
constexpr uint32_t kContinentMaskSamplerUnit = 4; // matches layout(binding=4) in height_earth.comp
} // namespace

HeightBuffers HeightStrategy::Run(const StrategyContext& sc,
                                  const PgBodyDesc& desc,
                                  GpuBufferHandle vertices)
{
    auto& gpu = sc.backend;

    // The macro layer has its own program variant (see height_earth.comp).
    const uint32_t program = sc.registry.Resolve(sc.macroField != 0 ? BuiltinProgram::HeightMacro
                                                                     : BuiltinProgram::Height);
    if (!program)
        return {};

    const size_t heightBytes = sc.vertexCount * sizeof(float);
    const size_t normalBytes = sc.vertexCount * 3 * sizeof(float);

    // Per-patch path passes app-owned output buffers; otherwise allocate our own.
    const GpuBufferHandle heightBuf = sc.outHeights ? sc.outHeights : gpu.CreateBuffer(heightBytes);
    const GpuBufferHandle normalBuf = sc.outNormals ? sc.outNormals : gpu.CreateBuffer(normalBytes);

    gpu.BindShader(program);
    gpu.BindBuffer(vertices,  0);
    gpu.BindBuffer(heightBuf, 1);
    gpu.BindBuffer(normalBuf, 2);

    auto params = MakeHeightParams(desc, sc.seed, sc.vertexCount);

    // Continent mask: bind the library-baked 3D texture to the sampler unit the
    // shader reads, and flag availability in the UBO (no loose uniforms).
    if (sc.continentMask != 0)
    {
        gpu.BindTexture3D(sc.continentMask, kContinentMaskSamplerUnit);
        params.continentMaskAvailable = 1;
    }

    // Macro world field: sampled by direction as the base elevation.
    if (sc.macroField != 0)
        gpu.BindTexture2DArray(sc.macroField, kMacroFieldSamplerUnit);

    const GpuBufferHandle ubo = gpu.CreateParamBuffer(sizeof(params));
    gpu.SetParams(ubo, &params, sizeof(params), kParamBinding);

    gpu.Dispatch(GroupCount(sc.vertexCount, HeightWorkgroupSize));
    gpu.Barrier();

    gpu.DestroyParamBuffer(ubo);

    return { heightBuf, normalBuf };
}

} // namespace planetgen
