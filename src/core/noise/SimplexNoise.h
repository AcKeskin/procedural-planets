#pragma once

#include "Noise.h"
#include <vector>
#include <cstdint>

namespace planets::core {

// 3D Simplex noise based on Ken Perlin's 2001 paper
class SimplexNoise : public INoiseGenerator
{
public:
    explicit SimplexNoise(uint32_t seed = 0);

    float Sample(float x, float y, float z) const override;

private:
    float Dot(const int* g, float x, float y, float z) const;
    int FastFloor(float x) const;

    std::vector<int> _perm;
    std::vector<int> _permMod12;

    static constexpr float F3 = 1.0f / 3.0f;
    static constexpr float G3 = 1.0f / 6.0f;

    static constexpr int Grad3[12][3] = {
        {1,1,0}, {-1,1,0}, {1,-1,0}, {-1,-1,0},
        {1,0,1}, {-1,0,1}, {1,0,-1}, {-1,0,-1},
        {0,1,1}, {0,-1,1}, {0,1,-1}, {0,-1,-1}
    };
};

} // namespace planets::core
