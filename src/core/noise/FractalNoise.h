#pragma once

#include "Noise.h"
#include <memory>

namespace planets::core {

class SimplexNoise;

// Fractal Brownian Motion - layers multiple octaves for multi-scale terrain
class FractalNoise : public INoiseGenerator
{
public:
    FractalNoise(
        uint32_t seed = 0,
        int octaves = 4,
        float persistence = 0.5f,
        float lacunarity = 2.0f);

    float Sample(float x, float y, float z) const override;

    void SetOctaves(int octaves) { _octaves = octaves; }
    void SetPersistence(float persistence) { _persistence = persistence; }
    void SetLacunarity(float lacunarity) { _lacunarity = lacunarity; }

private:
    std::unique_ptr<SimplexNoise> _baseNoise;
    int _octaves;
    float _persistence;
    float _lacunarity;
};

} // namespace planets::core
