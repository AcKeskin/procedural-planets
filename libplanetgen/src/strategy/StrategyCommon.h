#pragma once

#include <cstdint>

namespace planetgen
{

// Shared dispatch constants/helpers for the built-in strategies. Per-strategy
// workgroup sizes stay local to each strategy (they differ); only the binding
// point and the group-count math are common.

// UBO binding point for the param struct — SSBOs use bindings 0/1/2.
constexpr uint32_t kParamBinding = 3;

// Ceil-divide vertex count into dispatch groups.
inline uint32_t GroupCount(uint32_t count, uint32_t workgroupSize)
{
    return (count + workgroupSize - 1) / workgroupSize;
}

} // namespace planetgen
