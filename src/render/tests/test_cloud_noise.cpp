#include <catch2/catch_test_macros.hpp>

#include "materials/CloudNoise.h"

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <stdexcept>
#include <vector>

namespace
{

int At(const std::vector<uint8_t>& v, int size, int x, int y, int z)
{
    x = (x % size + size) % size;
    y = (y % size + size) % size;
    z = (z % size + size) % size;
    return v[(static_cast<size_t>(z) * size + y) * size + x];
}

// Mean step across the wrap edge vs the mean step between interior neighbours, along x.
void CheckTiles(const std::vector<uint8_t>& v, int size)
{
    double interior = 0.0, seam = 0.0;
    for (int z = 0; z < size; ++z)
        for (int y = 0; y < size; ++y)
        {
            for (int x = 0; x < size - 1; ++x)
                interior += std::abs(At(v, size, x + 1, y, z) - At(v, size, x, y, z));
            seam += std::abs(At(v, size, 0, y, z) - At(v, size, size - 1, y, z));
        }
    interior /= static_cast<double>(size) * size * (size - 1);
    seam /= static_cast<double>(size) * size;
    INFO("interior " << interior << " seam " << seam);
    CHECK(seam <= interior * 2.0 + 1.0);
}

} // namespace

TEST_CASE("cloud_noise: volumes are deterministic per seed and tile", "[cloud_noise]")
{
    const int n = 16;
    auto a = planets::render::GenerateCloudShapeNoise(5u, n);
    CHECK(a == planets::render::GenerateCloudShapeNoise(5u, n));
    CHECK(a != planets::render::GenerateCloudShapeNoise(6u, n));
    CheckTiles(a, n);

    auto d = planets::render::GenerateCloudDetailNoise(5u, n);
    CHECK(d == planets::render::GenerateCloudDetailNoise(5u, n));
    CheckTiles(d, n);
}

TEST_CASE("cloud_noise: shape noise spans a usable range", "[cloud_noise]")
{
    auto a = planets::render::GenerateCloudShapeNoise(9u, 16);
    int lo = 255, hi = 0;
    for (uint8_t v : a)
    {
        lo = std::min<int>(lo, v);
        hi = std::max<int>(hi, v);
    }
    INFO("lo " << lo << " hi " << hi);
    CHECK(hi - lo > 120);
}

TEST_CASE("cloud_noise: weather is deterministic and its coverage is spread evenly", "[cloud_noise]")
{
    const int f = 32;
    auto w = planets::render::GenerateCloudWeather(3u, f);
    REQUIRE(w.size() == static_cast<size_t>(6) * f * f * 2);
    CHECK(w == planets::render::GenerateCloudWeather(3u, f));

    // Equalized coverage: about half the sphere is below the midpoint, a fifth in each fifth.
    size_t below = 0, low = 0, high = 0;
    for (size_t i = 0; i < w.size(); i += 2)
    {
        below += w[i] < 128;
        low += w[i] < 51;
        high += w[i] >= 204;
    }
    const double texels = static_cast<double>(w.size() / 2);
    CHECK(std::abs(below / texels - 0.5) < 0.02);
    CHECK(std::abs(low / texels - 0.2) < 0.02);
    CHECK(std::abs(high / texels - 0.2) < 0.02);
}

TEST_CASE("cloud_noise: weather texel directions match the GL cube map faces", "[cloud_noise]")
{
    float d[3];
    planets::render::CloudWeatherTexelDir(0, 8, 8, 16, d); // centre of +X
    CHECK(d[0] > 0.99f);
    planets::render::CloudWeatherTexelDir(3, 8, 8, 16, d); // centre of -Y
    CHECK(d[1] < -0.99f);
    planets::render::CloudWeatherTexelDir(5, 8, 8, 16, d); // centre of -Z
    CHECK(d[2] < -0.99f);
}

TEST_CASE("cloud_noise: rejects sizes that are not powers of two", "[cloud_noise]")
{
    CHECK_THROWS_AS(planets::render::GenerateCloudShapeNoise(1u, 12), std::invalid_argument);
    CHECK_THROWS_AS(planets::render::GenerateCloudWeather(1u, 8), std::invalid_argument);
}
