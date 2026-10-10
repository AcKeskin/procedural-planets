#pragma once

// Noise for the volumetric cloud layer, generated from the body seed. Pure CPU functions: no GL,
// so they are deterministic and testable on their own; the renderer uploads the results.

#include <cstdint>
#include <vector>

namespace planetgen
{

// Tiling 3D noise volumes, size^3 bytes, x fastest. size must be a power of two >= 8.
// Shape: Perlin-Worley (billowy cloud bodies). Detail: Worley fBm (ragged edges).
std::vector<uint8_t> GenerateCloudShapeNoise(uint32_t seed, int size);
std::vector<uint8_t> GenerateCloudDetailNoise(uint32_t seed, int size);

// Weather on the sphere as a cube map: 6 faces (+X, -X, +Y, -Y, +Z, -Z in GL order), each
// faceSize^2 texels of RG8. R = coverage (0..1, before the global coverage setting), G = cloud
// type (0 = flat stratus, 1 = tall cumulus).
std::vector<uint8_t> GenerateCloudWeather(uint32_t seed, int faceSize);

// Direction of the centre of a cube-map texel, the same convention GL samples with.
void CloudWeatherTexelDir(int face, int x, int y, int faceSize, float out[3]);

} // namespace planetgen
