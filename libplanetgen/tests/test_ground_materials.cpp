#include <catch2/catch_test_macros.hpp>

#include "materials/GroundMaterials.h"

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <stdexcept>
#include <vector>

using planetgen::GenerateGroundMaterials;
using planetgen::GroundMaterialCount;

namespace
{

constexpr int kSize = 64;

// Height (alpha) of a texel, wrapping at the tile edge.
int HeightAt(const std::vector<uint8_t>& layer, int size, int x, int y)
{
    x = (x % size + size) % size;
    y = (y % size + size) % size;
    return layer[(static_cast<size_t>(y) * size + x) * 4 + 3];
}

} // namespace

TEST_CASE("ground_materials: same seed gives identical bytes, another seed differs", "[ground_materials]")
{
    auto a = GenerateGroundMaterials(42u, kSize);
    auto b = GenerateGroundMaterials(42u, kSize);
    auto c = GenerateGroundMaterials(43u, kSize);

    REQUIRE(a.detail.size() == static_cast<size_t>(GroundMaterialCount));
    for (int layer = 0; layer < GroundMaterialCount; ++layer)
    {
        CHECK(a.detail[layer] == b.detail[layer]);
        CHECK(a.normal[layer] == b.normal[layer]);
        CHECK(a.detail[layer] != c.detail[layer]);
    }
}

TEST_CASE("ground_materials: every layer tiles without a seam", "[ground_materials]")
{
    auto set = GenerateGroundMaterials(7u, kSize);
    for (int layer = 0; layer < GroundMaterialCount; ++layer)
    {
        const auto& d = set.detail[layer];
        // Height step across the wrap edge compared with the step between interior neighbours.
        double interior = 0.0, seam = 0.0;
        for (int y = 0; y < kSize; ++y)
        {
            for (int x = 0; x < kSize - 1; ++x)
                interior += std::abs(HeightAt(d, kSize, x + 1, y) - HeightAt(d, kSize, x, y));
            seam += std::abs(HeightAt(d, kSize, 0, y) - HeightAt(d, kSize, kSize - 1, y));
        }
        interior /= static_cast<double>(kSize) * (kSize - 1);
        seam /= static_cast<double>(kSize);
        INFO("layer " << layer << " interior " << interior << " seam " << seam);
        CHECK(seam <= interior * 2.0 + 1.0);
    }
}

TEST_CASE("ground_materials: albedo detail is centred on the layer mean", "[ground_materials]")
{
    auto set = GenerateGroundMaterials(3u, kSize);
    for (int layer = 0; layer < GroundMaterialCount; ++layer)
    {
        const auto& d = set.detail[layer];
        double sum = 0.0;
        for (size_t i = 0; i < d.size(); i += 4)
            sum += d[i + 1]; // green
        double mean = sum / (d.size() / 4);
        INFO("layer " << layer << " mean " << mean);
        CHECK(std::abs(mean - 127.5) < 6.0);
    }
}

TEST_CASE("ground_materials: rejects a size that is not a power of two", "[ground_materials]")
{
    CHECK_THROWS_AS(GenerateGroundMaterials(1u, 100), std::invalid_argument);
    CHECK_THROWS_AS(GenerateGroundMaterials(1u, 8), std::invalid_argument);
}
