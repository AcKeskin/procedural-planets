#pragma once

#include "planetgen/planetgen.h"
#include <nlohmann/json.hpp>
#include <cstdint>

namespace planetgen
{

// A planet type's `variety` block: dotted keys into the body JSON, each with how to draw it.
//   {"range": [a, b]}  uniform number (integer when both ends are integers)
//   {"pick": [v, ...]} one of the values, any JSON (a palette, a weather name)
//   {"mix": [a, b]}    one random blend of two numbers or two equal-size number arrays
// A world is a type plus a seed: the same body and seed always roll the same values.
PG_API bool HasVariety(const nlohmann::json& body);

// The body with every variety key set to its rolled value. Unknown forms are skipped.
PG_API nlohmann::json RollVariety(const nlohmann::json& body, uint32_t seed);

} // namespace planetgen
