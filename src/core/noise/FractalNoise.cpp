#include "FractalNoise.h"
#include "SimplexNoise.h"

namespace planets::core {

FractalNoise::FractalNoise(
    uint32_t seed,
    int octaves,
    float persistence,
    float lacunarity)
    : _octaves(octaves)
    , _persistence(persistence)
    , _lacunarity(lacunarity)
{
    _baseNoise = std::make_unique<SimplexNoise>(seed);
}

float FractalNoise::Sample(float x, float y, float z) const
{
    float total = 0.0f;
    float amplitude = 1.0f;
    float frequency = 1.0f;
    float maxValue = 0.0f;

    for (int i = 0; i < _octaves; ++i)
    {
        float noiseValue = _baseNoise->Sample(
            x * frequency,
            y * frequency,
            z * frequency
        );

        // Convert from [0,1] to [-1,1] for proper FBM
        noiseValue = noiseValue * 2.0f - 1.0f;

        total += noiseValue * amplitude;
        maxValue += amplitude;

        amplitude *= _persistence;
        frequency *= _lacunarity;
    }

    // Normalize back to [0, 1]
    return (total / maxValue + 1.0f) * 0.5f;
}

} // namespace planets::core
