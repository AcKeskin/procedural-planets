#include "SimplexNoise.h"
#include <algorithm>
#include <random>

namespace planets::core {

constexpr int SimplexNoise::Grad3[12][3];

SimplexNoise::SimplexNoise(uint32_t seed)
{
    _perm.resize(512);
    _permMod12.resize(512);

    std::vector<int> p(256);
    for (int i = 0; i < 256; ++i)
    {
        p[i] = i;
    }

    std::default_random_engine engine(seed);
    std::shuffle(p.begin(), p.end(), engine);

    for (int i = 0; i < 512; ++i)
    {
        _perm[i] = p[i & 255];
        _permMod12[i] = _perm[i] % 12;
    }
}

float SimplexNoise::Sample(float x, float y, float z) const
{
    float n0, n1, n2, n3;

    float s = (x + y + z) * F3;
    int i = FastFloor(x + s);
    int j = FastFloor(y + s);
    int k = FastFloor(z + s);

    float t = (i + j + k) * G3;
    float X0 = i - t;
    float Y0 = j - t;
    float Z0 = k - t;
    float x0 = x - X0;
    float y0 = y - Y0;
    float z0 = z - Z0;

    int i1, j1, k1;
    int i2, j2, k2;

    if (x0 >= y0)
    {
        if (y0 >= z0)
        {
            i1 = 1; j1 = 0; k1 = 0; i2 = 1; j2 = 1; k2 = 0;
        }
        else if (x0 >= z0)
        {
            i1 = 1; j1 = 0; k1 = 0; i2 = 1; j2 = 0; k2 = 1;
        }
        else
        {
            i1 = 0; j1 = 0; k1 = 1; i2 = 1; j2 = 0; k2 = 1;
        }
    }
    else
    {
        if (y0 < z0)
        {
            i1 = 0; j1 = 0; k1 = 1; i2 = 0; j2 = 1; k2 = 1;
        }
        else if (x0 < z0)
        {
            i1 = 0; j1 = 1; k1 = 0; i2 = 0; j2 = 1; k2 = 1;
        }
        else
        {
            i1 = 0; j1 = 1; k1 = 0; i2 = 1; j2 = 1; k2 = 0;
        }
    }

    float x1 = x0 - i1 + G3;
    float y1 = y0 - j1 + G3;
    float z1 = z0 - k1 + G3;
    float x2 = x0 - i2 + 2.0f * G3;
    float y2 = y0 - j2 + 2.0f * G3;
    float z2 = z0 - k2 + 2.0f * G3;
    float x3 = x0 - 1.0f + 3.0f * G3;
    float y3 = y0 - 1.0f + 3.0f * G3;
    float z3 = z0 - 1.0f + 3.0f * G3;

    int ii = i & 255;
    int jj = j & 255;
    int kk = k & 255;
    int gi0 = _permMod12[ii + _perm[jj + _perm[kk]]];
    int gi1 = _permMod12[ii + i1 + _perm[jj + j1 + _perm[kk + k1]]];
    int gi2 = _permMod12[ii + i2 + _perm[jj + j2 + _perm[kk + k2]]];
    int gi3 = _permMod12[ii + 1 + _perm[jj + 1 + _perm[kk + 1]]];

    float t0 = 0.6f - x0 * x0 - y0 * y0 - z0 * z0;
    if (t0 < 0)
    {
        n0 = 0.0f;
    }
    else
    {
        t0 *= t0;
        n0 = t0 * t0 * Dot(Grad3[gi0], x0, y0, z0);
    }

    float t1 = 0.6f - x1 * x1 - y1 * y1 - z1 * z1;
    if (t1 < 0)
    {
        n1 = 0.0f;
    }
    else
    {
        t1 *= t1;
        n1 = t1 * t1 * Dot(Grad3[gi1], x1, y1, z1);
    }

    float t2 = 0.6f - x2 * x2 - y2 * y2 - z2 * z2;
    if (t2 < 0)
    {
        n2 = 0.0f;
    }
    else
    {
        t2 *= t2;
        n2 = t2 * t2 * Dot(Grad3[gi2], x2, y2, z2);
    }

    float t3 = 0.6f - x3 * x3 - y3 * y3 - z3 * z3;
    if (t3 < 0)
    {
        n3 = 0.0f;
    }
    else
    {
        t3 *= t3;
        n3 = t3 * t3 * Dot(Grad3[gi3], x3, y3, z3);
    }

    // Scale to [0, 1]
    return (32.0f * (n0 + n1 + n2 + n3) + 1.0f) * 0.5f;
}

float SimplexNoise::Dot(const int* g, float x, float y, float z) const
{
    return g[0] * x + g[1] * y + g[2] * z;
}

int SimplexNoise::FastFloor(float x) const
{
    int xi = static_cast<int>(x);
    return x < xi ? xi - 1 : xi;
}

} // namespace planets::core
