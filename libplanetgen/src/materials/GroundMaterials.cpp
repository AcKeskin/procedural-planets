#include "GroundMaterials.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <thread>

namespace planetgen
{

namespace
{

struct Rgb
{
    float r, g, b;
};

Rgb Mix(const Rgb& a, const Rgb& b, float t)
{
    return {a.r + (b.r - a.r) * t, a.g + (b.g - a.g) * t, a.b + (b.b - a.b) * t};
}

Rgb Scale(const Rgb& c, float s)
{
    return {c.r * s, c.g * s, c.b * s};
}

float Clamp01(float v)
{
    return std::min(std::max(v, 0.0f), 1.0f);
}

float SmoothStep(float e0, float e1, float x)
{
    float t = Clamp01((x - e0) / (e1 - e0));
    return t * t * (3.0f - 2.0f * t);
}

uint32_t Hash(uint32_t x, uint32_t y, uint32_t salt)
{
    uint32_t h = x * 0x8da6b343u ^ y * 0xd8163841u ^ salt * 0xcb1ab31fu;
    h ^= h >> 16;
    h *= 0x7feb352du;
    h ^= h >> 15;
    h *= 0x846ca68bu;
    h ^= h >> 16;
    return h;
}

float Hash01(uint32_t x, uint32_t y, uint32_t salt)
{
    return static_cast<float>(Hash(x, y, salt) >> 8) * (1.0f / 16777216.0f);
}

uint32_t Wrap(int v, int period)
{
    return static_cast<uint32_t>(((v % period) + period) % period);
}

// Value noise on a lattice that repeats every `period` cells; (u, v) in [0,1) covers one period.
float PeriodicNoise(float u, float v, int period, uint32_t salt)
{
    float x = u * static_cast<float>(period);
    float y = v * static_cast<float>(period);
    int ix = static_cast<int>(std::floor(x));
    int iy = static_cast<int>(std::floor(y));
    float fx = x - static_cast<float>(ix);
    float fy = y - static_cast<float>(iy);
    fx = fx * fx * (3.0f - 2.0f * fx);
    fy = fy * fy * (3.0f - 2.0f * fy);
    float a = Hash01(Wrap(ix, period), Wrap(iy, period), salt);
    float b = Hash01(Wrap(ix + 1, period), Wrap(iy, period), salt);
    float c = Hash01(Wrap(ix, period), Wrap(iy + 1, period), salt);
    float e = Hash01(Wrap(ix + 1, period), Wrap(iy + 1, period), salt);
    return (a + (b - a) * fx) + ((c + (e - c) * fx) - (a + (b - a) * fx)) * fy;
}

// fbm with every octave on a lattice that still divides the tile, so the sum tiles too.
float PeriodicFbm(float u, float v, int basePeriod, int octaves, uint32_t salt, float gain = 0.5f)
{
    float sum = 0.0f;
    float amp = 0.5f;
    float norm = 0.0f;
    int period = basePeriod;
    for (int i = 0; i < octaves; ++i)
    {
        sum += amp * PeriodicNoise(u, v, period, salt + static_cast<uint32_t>(i) * 101u);
        norm += amp;
        amp *= gain;
        period *= 2;
    }
    return sum / norm;
}

struct Cell
{
    float f1;   // distance to the nearest feature point, in cells
    float edge; // distance to the border with the second nearest (0 on a crack)
    float id;   // random value of the nearest cell
};

// Cellular noise on a periodic grid: one jittered feature point per cell.
Cell PeriodicCells(float u, float v, int period, uint32_t salt)
{
    float x = u * static_cast<float>(period);
    float y = v * static_cast<float>(period);
    int ix = static_cast<int>(std::floor(x));
    int iy = static_cast<int>(std::floor(y));
    float best = 1e9f, second = 1e9f, bestId = 0.0f;
    for (int dy = -1; dy <= 1; ++dy)
    {
        for (int dx = -1; dx <= 1; ++dx)
        {
            uint32_t cx = Wrap(ix + dx, period);
            uint32_t cy = Wrap(iy + dy, period);
            float px = static_cast<float>(ix + dx) + Hash01(cx, cy, salt);
            float py = static_cast<float>(iy + dy) + Hash01(cx, cy, salt + 7u);
            float d = std::hypot(px - x, py - y);
            if (d < best)
            {
                second = best;
                best = d;
                bestId = Hash01(cx, cy, salt + 13u);
            }
            else if (d < second)
            {
                second = d;
            }
        }
    }
    return {best, second - best, bestId};
}

struct Texel
{
    Rgb color;
    float height; // 0..1
};

Texel Rock(float u, float v, uint32_t s)
{
    float warp = PeriodicFbm(u, v, 4, 4, s + 1u);
    float strata = 0.5f + 0.5f * std::sin((v + warp * 0.18f) * 2.0f * 3.14159265f * 12.0f);
    float grain = PeriodicFbm(u, v, 16, 5, s + 2u);
    // Warped cells so the cracks wander instead of reading as paving stones.
    float wu = PeriodicFbm(u, v, 5, 3, s + 6u) - 0.5f;
    float wv = PeriodicFbm(u, v, 5, 3, s + 7u) - 0.5f;
    Cell crack = PeriodicCells(u + wu * 0.12f, v + wv * 0.12f, 5, s + 3u);
    float crackLine = (1.0f - SmoothStep(0.0f, 0.035f, crack.edge)) * SmoothStep(0.35f, 0.6f, crack.id + 0.3f * wu);
    float height = Clamp01(0.55f * PeriodicFbm(u, v, 4, 6, s + 4u) + 0.25f * strata + 0.2f * grain
                           - 0.35f * crackLine);
    // No large stains here: they would show the tile repeat; the shader paints large-scale colour.
    Rgb light{0.46f, 0.44f, 0.41f}, dark{0.27f, 0.26f, 0.25f};
    Rgb color = Mix(dark, light, Clamp01(height * 1.2f));
    color = Scale(color, 1.0f - 0.35f * crackLine);
    return {color, height};
}

Texel Grass(float u, float v, uint32_t s)
{
    Cell clump = PeriodicCells(u, v, 24, s + 1u);
    float tuft = 1.0f - SmoothStep(0.0f, 0.7f, clump.f1);
    float blades = PeriodicFbm(u, v, 64, 3, s + 2u);
    float patchy = PeriodicFbm(u, v, 4, 5, s + 3u);
    float height = Clamp01(0.5f * tuft + 0.35f * blades + 0.15f * patchy);
    Rgb deep{0.06f, 0.15f, 0.04f}, mid{0.24f, 0.42f, 0.14f}, dry{0.46f, 0.45f, 0.22f};
    // Dry tips per clump only; tile-sized patches would show the repeat.
    Rgb color = Mix(deep, mid, height);
    color = Mix(color, dry, SmoothStep(0.75f, 1.0f, clump.id) * tuft * 0.5f);
    return {color, height};
}

Texel Sand(float u, float v, uint32_t s)
{
    float warp = PeriodicFbm(u, v, 3, 4, s + 1u);
    float ripple = 0.5f + 0.5f * std::sin((u + warp * 0.25f) * 2.0f * 3.14159265f * 18.0f);
    ripple = std::pow(ripple, 1.6f);
    float speckle = Hash01(static_cast<uint32_t>(u * 4096.0f), static_cast<uint32_t>(v * 4096.0f), s + 2u);
    float dunes = PeriodicFbm(u, v, 2, 4, s + 3u);
    float height = Clamp01(0.55f * ripple + 0.35f * dunes + 0.1f * speckle);
    Rgb a{0.66f, 0.56f, 0.40f}, b{0.78f, 0.69f, 0.50f};
    Rgb color = Mix(a, b, height);
    color = Scale(color, 0.9f + 0.2f * speckle);
    return {color, height};
}

Texel Snow(float u, float v, uint32_t s)
{
    float drifts = PeriodicFbm(u * 1.0f, v, 3, 5, s + 1u);
    float wind = PeriodicFbm(u, v, 8, 3, s + 2u);
    float height = Clamp01(0.75f * drifts + 0.25f * wind);
    Rgb shade{0.80f, 0.85f, 0.93f}, lit{0.96f, 0.97f, 1.0f};
    return {Mix(shade, lit, height), height};
}

Texel Soil(float u, float v, uint32_t s)
{
    Cell mud = PeriodicCells(u, v, 10, s + 1u);
    // Only some cells crack, and faintly: strong cracks read as veins from a distance.
    float crack = (1.0f - SmoothStep(0.0f, 0.04f, mud.edge)) * SmoothStep(0.4f, 0.7f, mud.id);
    float plate = PeriodicFbm(u, v, 8, 4, s + 2u);
    Cell pebble = PeriodicCells(u, v, 48, s + 3u);
    float stone = (1.0f - SmoothStep(0.15f, 0.3f, pebble.f1)) * (pebble.id > 0.7f ? 1.0f : 0.0f);
    float height = Clamp01(0.6f * plate + 0.3f * mud.id + 0.3f * stone - 0.5f * crack);
    Rgb dry{0.45f, 0.36f, 0.26f}, dark{0.30f, 0.23f, 0.16f}, gravel{0.50f, 0.48f, 0.44f};
    Rgb color = Mix(dark, dry, Clamp01(plate * 0.8f + mud.id * 0.3f));
    color = Mix(color, gravel, stone);
    color = Scale(color, 1.0f - 0.25f * crack);
    return {color, height};
}

Texel Sample(GroundMaterial m, float u, float v, uint32_t seed)
{
    uint32_t s = seed * 7919u + static_cast<uint32_t>(m) * 104729u;
    switch (m)
    {
    case GroundMaterial::Rock:
        return Rock(u, v, s);
    case GroundMaterial::Grass:
        return Grass(u, v, s);
    case GroundMaterial::Sand:
        return Sand(u, v, s);
    case GroundMaterial::Snow:
        return Snow(u, v, s);
    case GroundMaterial::Soil:
        return Soil(u, v, s);
    default:
        return {{0.5f, 0.5f, 0.5f}, 0.5f};
    }
}

uint8_t ToByte(float v)
{
    return static_cast<uint8_t>(std::lround(Clamp01(v) * 255.0f));
}

// One layer: colour + height per texel, then albedo relative to the mean and normals from the
// tiling height (wrap-around central differences).
void BuildLayer(GroundMaterial m, uint32_t seed, int size, std::vector<uint8_t>& detail, std::vector<uint8_t>& normal)
{
    const size_t count = static_cast<size_t>(size) * size;
    std::vector<Texel> texels(count);
    const float inv = 1.0f / static_cast<float>(size);
    for (int y = 0; y < size; ++y)
        for (int x = 0; x < size; ++x)
            texels[static_cast<size_t>(y) * size + x]
                = Sample(m, (static_cast<float>(x) + 0.5f) * inv, (static_cast<float>(y) + 0.5f) * inv, seed);

    double mr = 0.0, mg = 0.0, mb = 0.0;
    for (const Texel& t : texels)
    {
        mr += t.color.r;
        mg += t.color.g;
        mb += t.color.b;
    }
    const float meanR = static_cast<float>(mr / count), meanG = static_cast<float>(mg / count),
                meanB = static_cast<float>(mb / count);

    // Slope scale: height 0..1 across one tile is a gentle relief; this sets the bump depth.
    const float bumpDepth = 6.0f;
    detail.resize(count * 4);
    normal.resize(count * 4);
    for (int y = 0; y < size; ++y)
    {
        for (int x = 0; x < size; ++x)
        {
            const size_t i = static_cast<size_t>(y) * size + x;
            const Texel& t = texels[i];
            detail[i * 4 + 0] = ToByte(0.5f * t.color.r / meanR);
            detail[i * 4 + 1] = ToByte(0.5f * t.color.g / meanG);
            detail[i * 4 + 2] = ToByte(0.5f * t.color.b / meanB);
            detail[i * 4 + 3] = ToByte(t.height);

            auto h = [&](int xx, int yy)
            { return texels[static_cast<size_t>(Wrap(yy, size)) * size + Wrap(xx, size)].height; };
            float dx = (h(x + 1, y) - h(x - 1, y)) * 0.5f * bumpDepth;
            float dy = (h(x, y + 1) - h(x, y - 1)) * 0.5f * bumpDepth;
            float len = std::sqrt(dx * dx + dy * dy + 1.0f);
            normal[i * 4 + 0] = ToByte(-dx / len * 0.5f + 0.5f);
            normal[i * 4 + 1] = ToByte(-dy / len * 0.5f + 0.5f);
            normal[i * 4 + 2] = ToByte(1.0f / len * 0.5f + 0.5f);
            normal[i * 4 + 3] = 255;
        }
    }
}

} // namespace

GroundMaterialSet GenerateGroundMaterials(uint32_t seed, int size)
{
    if (size < 16 || (size & (size - 1)) != 0)
        throw std::invalid_argument("GenerateGroundMaterials: size must be a power of two >= 16");

    GroundMaterialSet set;
    set.size = size;
    set.detail.resize(GroundMaterialCount);
    set.normal.resize(GroundMaterialCount);

    // One thread per layer: layers are independent and each texel is a pure function, so the
    // output does not depend on scheduling.
    std::vector<std::thread> workers;
    for (int layer = 0; layer < GroundMaterialCount; ++layer)
    {
        workers.emplace_back([&set, seed, size, layer]()
                             { BuildLayer(static_cast<GroundMaterial>(layer), seed, size, set.detail[layer], set.normal[layer]); });
    }
    for (std::thread& w : workers)
        w.join();
    return set;
}

} // namespace planetgen
