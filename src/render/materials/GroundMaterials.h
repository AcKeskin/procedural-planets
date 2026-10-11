#pragma once

// Tiling ground material textures (rock, grass, sand, snow, soil), generated from the body seed.
// Pure CPU function: no GL, so it is deterministic and testable on its own; the renderer uploads
// the result into a mipmapped texture array.

#include <cstdint>
#include <vector>

namespace planets::render
{

enum class GroundMaterial : int
{
    Rock = 0,
    Grass,
    Sand,
    Snow,
    Soil,
    Count
};

constexpr int GroundMaterialCount = static_cast<int>(GroundMaterial::Count);

struct GroundMaterialSet
{
    int size = 0; // texels per side, a power of two; every layer tiles seamlessly

    // Per layer, size*size RGBA8 texels.
    // detail: rgb = albedo relative to the layer mean, stored as 0.5 * color / mean (mean -> 0.5);
    //         a = height. The shader multiplies its own material colour by 2 * rgb, so the
    //         texture adds detail without overriding biome colour.
    // normal: rgb = tangent-space normal * 0.5 + 0.5 (z up), a = 255.
    std::vector<std::vector<uint8_t>> detail;
    std::vector<std::vector<uint8_t>> normal;
};

// size must be a power of two >= 16. Same seed and size give byte-identical output.
GroundMaterialSet GenerateGroundMaterials(uint32_t seed, int size);

} // namespace planets::render
