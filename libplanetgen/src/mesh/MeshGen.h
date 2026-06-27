#pragma once

// Minimal library-side mesh generator for the optional mesh-producing public call.
// Produces a unit-sphere UV grid (positions + triangle indices) at a given
// resolution. Deliberately small and self-contained: it gives consumers without
// their own geometry a "mesh + per-vertex data + palette" path, without dragging
// app-side LOD/streaming into the library.

#include <cstdint>
#include <vector>

namespace planetgen
{

struct MeshData
{
    std::vector<float>    positions; // packed float3 (x,y,z per vertex), unit sphere
    std::vector<uint32_t> indices;   // triangle list
};

// Build a UV-sphere with `rings` latitude bands and `segments` longitude slices.
// Both are clamped to a sane minimum. Vertices = (rings+1)*(segments+1).
MeshData MakeUvSphere(uint32_t rings, uint32_t segments);

} // namespace planetgen
