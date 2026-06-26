#pragma once

// Maps flat C-API descriptor structs to GPU-ready std140 param blocks.
// Used by both the library's own dispatch (planetgen_api.cpp) and the app's
// render layer (BodyRuntime) so the field mapping lives in one place.

#include "ParamBlocks.h"
#include "planetgen/planetgen.h"
#include <cstdint>

namespace planetgen
{

HeightParams        MakeHeightParams(const PgBodyDesc& desc, uint32_t seed, uint32_t vertexCount);
ShadingEarthParams  MakeShadingEarthParams(const PgShadingDesc& desc, uint32_t seed, uint32_t vertexCount);
ShadingGenericParams MakeShadingGenericParams(const PgShadingDesc& desc, uint32_t seed, uint32_t vertexCount);
ErosionParams       MakeErosionParams(const PgErosionDesc& desc, uint32_t vertexCount);

} // namespace planetgen
