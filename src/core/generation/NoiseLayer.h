#pragma once

#include "../noise/Noise.h"
#include <memory>
#include <cstdint>

namespace planets::core
{

enum class NoiseType
{
    Simplex,
    Fractal
};

struct NoiseLayerSettings
{
    NoiseType type = NoiseType::Fractal;
    float scale = 1.0f;
    float strength = 1.0f;
    int octaves = 4;
    float persistence = 0.5f;
    float lacunarity = 2.0f;
    float verticalShift = 0.0f;
    uint32_t seed = 0;
    bool enabled = true;
    bool useMask = false;
};

// Configurable noise layer for terrain generation
class NoiseLayer
{
public:
    NoiseLayer();
    explicit NoiseLayer(const NoiseLayerSettings& settings);

    void Configure(const NoiseLayerSettings& settings);

    // Sample height at normalized direction, returns displacement value
    float Sample(float x, float y, float z) const;

    NoiseLayerSettings& GetSettings() { return _settings; }
    const NoiseLayerSettings& GetSettings() const { return _settings; }

    bool IsEnabled() const { return _settings.enabled; }
    bool UsesMask() const { return _settings.useMask; }

private:
    void CreateGenerator();

    NoiseLayerSettings _settings;
    std::unique_ptr<INoiseGenerator> _generator;
};

} // namespace planets::core
