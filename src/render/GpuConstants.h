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

} // namespace planets::render
