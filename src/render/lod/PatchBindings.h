#pragma once

namespace planets::render::lod
{

// SSBO binding points for the GPU-resident patch draw path. The vertex shader indexes
// these by gl_VertexID; the draw site binds each patch's buffer to the matching point.
// Keep in lockstep with the layout(std430, binding=N) lines in the patch vertex shaders —
// a mismatch silently reads the wrong buffer (no GL error).
enum PatchBinding : unsigned int
{
    kPatchPositionBinding = 0, // unit-sphere positions, float3 per vertex
    kPatchHeightBinding = 1,   // height multiplier, float per vertex
    kPatchNormalBinding = 2,   // analytical normal, float3 per vertex
    kPatchShadingBinding = 3,  // shading data, float4 per vertex
};

} // namespace planets::render::lod
