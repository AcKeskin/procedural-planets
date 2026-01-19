#pragma once

#include "NoiseLayer.h"
#include <vector>
#include <cstdint>

namespace planets::core {

struct PlanetSettings
{
    float radius = 1.0f;
    float seaLevel = 0.4f;
    float heightScale = 0.05f;
    int subdivisions = 4;
    uint32_t seed = 0;
};

// Planet data model with noise layers for terrain generation
class Planet
{
public:
    Planet();
    explicit Planet(const PlanetSettings& settings);

    void Configure(const PlanetSettings& settings);

    // Sample terrain height at normalized direction
    // Returns height in range [0, 1] before scaling
    float SampleHeight(float x, float y, float z) const;

    // Get final radius at point (includes height displacement)
    float GetRadiusAt(float x, float y, float z) const;

    // Noise layer management
    void AddNoiseLayer(const NoiseLayerSettings& settings);
    void ClearNoiseLayers();
    NoiseLayer& GetNoiseLayer(size_t index) { return _noiseLayers[index]; }
    size_t GetNoiseLayerCount() const { return _noiseLayers.size(); }

    PlanetSettings& GetSettings() { return _settings; }
    const PlanetSettings& GetSettings() const { return _settings; }

    // Regenerate with new seed
    void Reseed(uint32_t seed);

private:
    PlanetSettings _settings;
    std::vector<NoiseLayer> _noiseLayers;
};

} // namespace planets::core
