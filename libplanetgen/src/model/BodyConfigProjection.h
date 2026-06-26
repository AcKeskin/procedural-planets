#pragma once

// Projection from the typed BodyConfig model down to the flat C-ABI structs.
// The flat structs are outputs only — they carry no independent defaults.

#include "BodyConfig.h"
#include "planetgen/planetgen.h"

namespace planetgen
{

PgBodyDesc    ProjectToBodyDesc(const BodyConfig& cfg);
PgShadingDesc ProjectToShadingDesc(const BodyConfig& cfg);
PgErosionDesc ProjectToErosionDesc(const BodyConfig& cfg);

} // namespace planetgen
