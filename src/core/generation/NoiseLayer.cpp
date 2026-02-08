#include "NoiseLayer.h"
#include "../noise/SimplexNoise.h"
#include "../noise/FractalNoise.h"

namespace planets::core
{

NoiseLayer::NoiseLayer()
{
    CreateGenerator();
}

NoiseLayer::NoiseLayer(const NoiseLayerSettings& settings)
    : _settings(settings)
{
    CreateGenerator();
}

void NoiseLayer::Configure(const NoiseLayerSettings& settings)
{
    _settings = settings;
    CreateGenerator();
}

float NoiseLayer::Sample(float x, float y, float z) const
{
    if (!_settings.enabled || !_generator)
    {
        return 0.0f;
    }

    float scaledX = x * _settings.scale;
    float scaledY = y * _settings.scale;
    float scaledZ = z * _settings.scale;

    float noise = _generator->Sample(scaledX, scaledY, scaledZ);

    // Apply strength and vertical shift
    return (noise * _settings.strength) + _settings.verticalShift;
}

void NoiseLayer::CreateGenerator()
{
    switch (_settings.type)
    {
    case NoiseType::Simplex:
        _generator = std::make_unique<SimplexNoise>(_settings.seed);
        break;

    case NoiseType::Fractal:
    default:
        _generator = std::make_unique<FractalNoise>(
            _settings.seed, _settings.octaves, _settings.persistence, _settings.lacunarity);
        break;
    }
}

} // namespace planets::core
