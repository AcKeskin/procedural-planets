#pragma once

#include "IGenerationStrategy.h"

namespace planetgen
{

// Built-in shading stage. Picks the earth (climate) vs generic shader by
// desc.use_climate_model internally — callers never make that choice.
class ShadingStrategy final : public IShadingStrategy
{
public:
    GpuBufferHandle Run(const StrategyContext& sc,
                        const PgShadingDesc& desc,
                        GpuBufferHandle vertices,
                        GpuBufferHandle heights) override;
};

} // namespace planetgen
