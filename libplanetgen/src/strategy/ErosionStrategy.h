#pragma once

#include "IGenerationStrategy.h"

namespace planetgen
{

// Built-in erosion stage: iterates the Erosion program, ping-ponging two height
// buffers. Returns the authoritative heights buffer after the last iteration.
class ErosionStrategy final : public IErosionStrategy
{
public:
    GpuBufferHandle Run(const StrategyContext& sc,
                        const PgErosionDesc& desc,
                        GpuBufferHandle heights) override;
};

} // namespace planetgen
