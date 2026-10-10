#include "CloudNoise.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <thread>

namespace planetgen
{

namespace
{

uint32_t Hash(uint32_t x, uint32_t y, uint32_t z, uint32_t salt)
{
    uint32_t h = x * 0x8da6b343u ^ y * 0xd8163841u ^ z * 0xcb1ab31fu ^ salt * 0x165667b1u;
    h ^= h >> 16;
    h *= 0x7feb352du;
    h ^= h >> 15;
    h *= 0x846ca68bu;
    h ^= h >> 16;
    return h;
}

float Hash01(uint32_t x, uint32_t y, uint32_t z, uint32_t salt)
{
    return static_cast<float>(Hash(x, y, z, salt) >> 8) * (1.0f / 16777216.0f);
}

uint32_t Wrap(int v, int period)
{
    return static_cast<uint32_t>(((v % period) + period) % period);
}

float Clamp01(float v)
{
    return std::min(std::max(v, 0.0f), 1.0f);
}

float Remap(float v, float lo, float hi, float newLo, float newHi)
{
    return newLo + (v - lo) / (hi - lo) * (newHi - newLo);
}

// Gradient (Perlin) noise on a lattice that repeats every `period` cells. p in cells. ~[-1, 1].
float PeriodicPerlin(float px, float py, float pz, int period, uint32_t salt)
{
    int ix = static_cast<int>(std::floor(px)), iy = static_cast<int>(std::floor(py)), iz = static_cast<int>(std::floor(pz));
    float fx = px - ix, fy = py - iy, fz = pz - iz;
    auto grad = [&](int dx, int dy, int dz)
    {
        uint32_t h = Hash(Wrap(ix + dx, period), Wrap(iy + dy, period), Wrap(iz + dz, period), salt);
        // 12 edge directions of a cube.
        static const float g[12][3] = {{1, 1, 0}, {-1, 1, 0}, {1, -1, 0}, {-1, -1, 0}, {1, 0, 1}, {-1, 0, 1},
                                       {1, 0, -1}, {-1, 0, -1}, {0, 1, 1}, {0, -1, 1}, {0, 1, -1}, {0, -1, -1}};
        const float* v = g[h % 12];
        return v[0] * (fx - dx) + v[1] * (fy - dy) + v[2] * (fz - dz);
    };
    auto fade = [](float t) { return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f); };
    float u = fade(fx), v = fade(fy), w = fade(fz);
    float x00 = grad(0, 0, 0) + (grad(1, 0, 0) - grad(0, 0, 0)) * u;
    float x10 = grad(0, 1, 0) + (grad(1, 1, 0) - grad(0, 1, 0)) * u;
    float x01 = grad(0, 0, 1) + (grad(1, 0, 1) - grad(0, 0, 1)) * u;
    float x11 = grad(0, 1, 1) + (grad(1, 1, 1) - grad(0, 1, 1)) * u;
    float y0 = x00 + (x10 - x00) * v;
    float y1 = x01 + (x11 - x01) * v;
    return y0 + (y1 - y0) * w;
}

// Inverted Worley (1 at feature points, 0 between) on a periodic grid. p in cells.
float PeriodicWorley(float px, float py, float pz, int period, uint32_t salt)
{
    int ix = static_cast<int>(std::floor(px)), iy = static_cast<int>(std::floor(py)), iz = static_cast<int>(std::floor(pz));
    float best = 1e9f;
    for (int dz = -1; dz <= 1; ++dz)
        for (int dy = -1; dy <= 1; ++dy)
            for (int dx = -1; dx <= 1; ++dx)
            {
                uint32_t cx = Wrap(ix + dx, period), cy = Wrap(iy + dy, period), cz = Wrap(iz + dz, period);
                float fx = ix + dx + Hash01(cx, cy, cz, salt) - px;
                float fy = iy + dy + Hash01(cx, cy, cz, salt + 1u) - py;
                float fz = iz + dz + Hash01(cx, cy, cz, salt + 2u) - pz;
                best = std::min(best, fx * fx + fy * fy + fz * fz);
            }
    return 1.0f - Clamp01(std::sqrt(best));
}

// Worley fBm: three octaves at doubling frequency, each still dividing the tile.
float WorleyFbm(float u, float v, float w, int basePeriod, uint32_t salt)
{
    float a = PeriodicWorley(u * basePeriod, v * basePeriod, w * basePeriod, basePeriod, salt);
    float b = PeriodicWorley(u * basePeriod * 2, v * basePeriod * 2, w * basePeriod * 2, basePeriod * 2, salt + 7u);
    float c = PeriodicWorley(u * basePeriod * 4, v * basePeriod * 4, w * basePeriod * 4, basePeriod * 4, salt + 13u);
    return a * 0.625f + b * 0.25f + c * 0.125f;
}

float PerlinFbm(float u, float v, float w, int basePeriod, uint32_t salt)
{
    float sum = 0.0f, amp = 0.5f, norm = 0.0f;
    int period = basePeriod;
    for (int i = 0; i < 4; ++i)
    {
        sum += amp * PeriodicPerlin(u * period, v * period, w * period, period, salt + i * 31u);
        norm += amp;
        amp *= 0.5f;
        period *= 2;
    }
    return sum / norm; // ~[-1, 1]
}

void CheckSize(int size, int minimum)
{
    if (size < minimum || (size & (size - 1)) != 0)
        throw std::invalid_argument("cloud noise: size must be a power of two");
}

// Fills size^3 texels with fn(u, v, w), one thread per slab of z.
template <typename Fn> std::vector<uint8_t> FillVolume(int size, Fn fn)
{
    std::vector<uint8_t> out(static_cast<size_t>(size) * size * size);
    const unsigned threads = std::max(1u, std::min(8u, std::thread::hardware_concurrency()));
    std::vector<std::thread> workers;
    for (unsigned t = 0; t < threads; ++t)
    {
        workers.emplace_back(
            [&, t]()
            {
                for (int z = static_cast<int>(t); z < size; z += static_cast<int>(threads))
                    for (int y = 0; y < size; ++y)
                        for (int x = 0; x < size; ++x)
                        {
                            float u = (x + 0.5f) / size, v = (y + 0.5f) / size, w = (z + 0.5f) / size;
                            float value = Clamp01(fn(u, v, w));
                            out[(static_cast<size_t>(z) * size + y) * size + x]
                                = static_cast<uint8_t>(std::lround(value * 255.0f));
                        }
            });
    }
    for (auto& w : workers)
        w.join();
    return out;
}

// Stretches the volume to the full byte range, so the shader's remap thresholds mean the same
// thing whatever the noise's natural range is.
std::vector<uint8_t> Normalize(std::vector<uint8_t> v)
{
    auto [lo, hi] = std::minmax_element(v.begin(), v.end());
    const int a = *lo, b = *hi;
    if (b > a)
        for (uint8_t& x : v)
            x = static_cast<uint8_t>((static_cast<int>(x) - a) * 255 / (b - a));
    return v;
}

// Plain (non-tiling) 3D value noise for the weather map, which lives on the sphere directly.
float ValueNoise(float x, float y, float z, uint32_t salt)
{
    int ix = static_cast<int>(std::floor(x)), iy = static_cast<int>(std::floor(y)), iz = static_cast<int>(std::floor(z));
    float fx = x - ix, fy = y - iy, fz = z - iz;
    fx = fx * fx * (3 - 2 * fx);
    fy = fy * fy * (3 - 2 * fy);
    fz = fz * fz * (3 - 2 * fz);
    auto h = [&](int dx, int dy, int dz)
    {
        return Hash01(static_cast<uint32_t>(ix + dx), static_cast<uint32_t>(iy + dy), static_cast<uint32_t>(iz + dz), salt);
    };
    float x00 = h(0, 0, 0) + (h(1, 0, 0) - h(0, 0, 0)) * fx;
    float x10 = h(0, 1, 0) + (h(1, 1, 0) - h(0, 1, 0)) * fx;
    float x01 = h(0, 0, 1) + (h(1, 0, 1) - h(0, 0, 1)) * fx;
    float x11 = h(0, 1, 1) + (h(1, 1, 1) - h(0, 1, 1)) * fx;
    float y0 = x00 + (x10 - x00) * fy, y1 = x01 + (x11 - x01) * fy;
    return y0 + (y1 - y0) * fz;
}

float ValueFbm(float x, float y, float z, int octaves, uint32_t salt)
{
    float sum = 0.0f, amp = 0.5f, norm = 0.0f, f = 1.0f;
    for (int i = 0; i < octaves; ++i)
    {
        sum += amp * ValueNoise(x * f, y * f, z * f, salt + i * 17u);
        norm += amp;
        amp *= 0.5f;
        f *= 2.03f;
    }
    return sum / norm;
}

} // namespace

std::vector<uint8_t> GenerateCloudShapeNoise(uint32_t seed, int size)
{
    CheckSize(size, 8);
    const uint32_t s = seed * 2654435761u + 11u;
    return Normalize(FillVolume(size,
                                [s](float u, float v, float w)
                                {
                                    // Perlin-Worley: Worley carves billows into the Perlin base.
                                    float perlin = PerlinFbm(u, v, w, 4, s) * 0.5f + 0.5f;
                                    float worley = WorleyFbm(u, v, w, 4, s + 100u);
                                    return Remap(perlin, worley - 1.0f, 1.0f, 0.0f, 1.0f);
                                }));
}

std::vector<uint8_t> GenerateCloudDetailNoise(uint32_t seed, int size)
{
    CheckSize(size, 8);
    const uint32_t s = seed * 2246822519u + 29u;
    return Normalize(FillVolume(size, [s](float u, float v, float w) { return WorleyFbm(u, v, w, 4, s); }));
}

void CloudWeatherTexelDir(int face, int x, int y, int faceSize, float out[3])
{
    // GL cube map face orientation (sc, tc in [-1, 1]).
    float sc = (2.0f * (x + 0.5f) / faceSize) - 1.0f;
    float tc = (2.0f * (y + 0.5f) / faceSize) - 1.0f;
    float d[3];
    switch (face)
    {
    case 0: d[0] = 1; d[1] = -tc; d[2] = -sc; break;
    case 1: d[0] = -1; d[1] = -tc; d[2] = sc; break;
    case 2: d[0] = sc; d[1] = 1; d[2] = tc; break;
    case 3: d[0] = sc; d[1] = -1; d[2] = -tc; break;
    case 4: d[0] = sc; d[1] = -tc; d[2] = 1; break;
    default: d[0] = -sc; d[1] = -tc; d[2] = -1; break;
    }
    float len = std::sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);
    out[0] = d[0] / len;
    out[1] = d[1] / len;
    out[2] = d[2] / len;
}

std::vector<uint8_t> GenerateCloudWeather(uint32_t seed, int faceSize)
{
    CheckSize(faceSize, 16);
    const uint32_t s = seed * 3266489917u + 47u;
    std::vector<uint8_t> out(static_cast<size_t>(6) * faceSize * faceSize * 2);
    std::vector<std::thread> workers;
    for (int face = 0; face < 6; ++face)
    {
        workers.emplace_back(
            [&, face]()
            {
                for (int y = 0; y < faceSize; ++y)
                    for (int x = 0; x < faceSize; ++x)
                    {
                        float d[3];
                        CloudWeatherTexelDir(face, x, y, faceSize, d);
                        // Domain warp so systems swirl instead of sitting in round blobs.
                        float wx = ValueFbm(d[0] * 2.0f + 11.3f, d[1] * 2.0f, d[2] * 2.0f, 3, s + 1u) - 0.5f;
                        float wy = ValueFbm(d[0] * 2.0f, d[1] * 2.0f + 7.1f, d[2] * 2.0f, 3, s + 2u) - 0.5f;
                        float wz = ValueFbm(d[0] * 2.0f, d[1] * 2.0f, d[2] * 2.0f + 3.7f, 3, s + 3u) - 0.5f;
                        float px = d[0] + wx * 0.6f, py = d[1] + wy * 0.6f, pz = d[2] + wz * 0.6f;
                        // Cloud systems (large) times individual cloud fields (medium).
                        float systems = ValueFbm(px * 2.2f, py * 2.2f, pz * 2.2f, 4, s + 10u);
                        float fields = ValueFbm(px * 9.0f, py * 9.0f, pz * 9.0f, 4, s + 20u);
                        float coverage = Clamp01((systems - 0.3f) * 2.2f) * Clamp01(fields * 1.6f - 0.15f);
                        float type = Clamp01((ValueFbm(px * 3.0f, py * 3.0f, pz * 3.0f, 3, s + 30u) - 0.35f) * 3.0f);
                        size_t i = ((static_cast<size_t>(face) * faceSize + y) * faceSize + x) * 2;
                        out[i + 0] = static_cast<uint8_t>(std::lround(Clamp01(coverage) * 255.0f));
                        out[i + 1] = static_cast<uint8_t>(std::lround(type * 255.0f));
                    }
            });
    }
    for (auto& w : workers)
        w.join();
    return out;
}

} // namespace planetgen
