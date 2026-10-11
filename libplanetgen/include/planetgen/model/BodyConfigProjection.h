#pragma once

// Projection from the typed BodyConfig model down to the flat C-ABI structs.
// The flat structs are outputs only — they carry no independent defaults.
// BodyConfigFromDesc goes the other way for pg_body_create, so a body made from a
// desc has a config like any other body.

#include "BodyConfig.h"
#include "planetgen/planetgen.h"

namespace planetgen
{

PG_API PgBodyDesc    ProjectToBodyDesc(const BodyConfig& cfg);
PG_API PgShadingDesc ProjectToShadingDesc(const BodyConfig& cfg);
PG_API PgErosionDesc ProjectToErosionDesc(const BodyConfig& cfg);

// Every desc field goes into the config; the blocks the desc does not carry keep
// their defaults. ProjectToBodyDesc(BodyConfigFromDesc(d)) gives d back.
PG_API BodyConfig BodyConfigFromDesc(const PgBodyDesc& d);

} // namespace planetgen
