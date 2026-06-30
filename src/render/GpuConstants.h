#pragma once

namespace planets::render
{

inline constexpr int ShaderInfoLogSize = 512;
inline constexpr unsigned int HeightWorkgroupSize = 512;
inline constexpr unsigned int ShadingWorkgroupSize = 256;
inline constexpr unsigned int ErosionWorkgroupSize = 256;
inline constexpr int FullscreenTriangleVertices = 3;

// SSBO binding points for fragment shader resources
inline constexpr unsigned int BiomePaletteBindingPoint = 3;

// Surface fresnel rim — near/far strength + falloff.
inline constexpr float FresnelStrengthNear = 0.5f;
inline constexpr float FresnelStrengthFar = 2.0f;
inline constexpr float FresnelPower = 3.0f;

// Async LOD generation budget per frame, and the ocean mesh subdivision level.
// 16/frame so detail resolves quickly when the camera approaches (and headless captures,
// which render a fixed warm-up window, settle to full subdivision before the shot).
inline constexpr int SchedulerPatchesPerFrame = 16;
inline constexpr int OceanSubdivisions = 5;

} // namespace planets::render
