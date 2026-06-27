#pragma once

#include "IGenerationStrategy.h"

namespace planetgen
{

// Built-in height stage: one compute pass over the Height program, producing
// heights + normals. Mirrors the dispatch that planetgen_api.cpp's
// pg_generate_heights ran imperatively, lifted behind IHeightStrategy.
class HeightStrategy final : public IHeightStrategy
{
public:
    HeightBuffers Run(const StrategyContext& sc,
                      const PgBodyDesc& desc,
                      GpuBufferHandle vertices) override;
};

} // namespace planetgen
