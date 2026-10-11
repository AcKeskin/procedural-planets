#pragma once

// JSON serialization/deserialization for BodyConfig.
// Load fills unset blocks with per-block defaults then validates.
// Serialize uses deterministic (alphabetical) key ordering.

#include "BodyConfig.h"
#include <string>

namespace planetgen
{

// Parse a BodyConfig from a JSON string.
// Returns empty-string error on success; non-empty string with reason on failure.
PG_API std::string BodyConfigFromJson(const std::string& json, BodyConfig& out);

// Serialize a BodyConfig to JSON string (deterministic key ordering).
PG_API std::string BodyConfigToJson(const BodyConfig& cfg);

// Convenience: load from filesystem path.
PG_API std::string BodyConfigLoadFile(const std::string& path, BodyConfig& out);

} // namespace planetgen
