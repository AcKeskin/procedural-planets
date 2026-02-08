#include "Planet.h"
#include <algorithm>

namespace planets::core
{

Planet::Planet()
{
    // Add default noise layer
    NoiseLayerSettings defaultLayer;
    defaultLayer.type = NoiseType::Fractal;
    defaultLayer.scale = 2.0f;
    defaultLayer.strength = 1.0f;
    defaultLayer.octaves = 6;
    AddNoiseLayer(defaultLayer);
}

Planet::Planet(const PlanetSettings& settings)
    : _settings(settings)
{
    NoiseLayerSettings defaultLayer;
    defaultLayer.type = NoiseType::Fractal;
    defaultLayer.scale = 2.0f;
    defaultLayer.strength = 1.0f;
    defaultLayer.octaves = 6;
    defaultLayer.seed = settings.seed;
    AddNoiseLayer(defaultLayer);
}

void Planet::Configure(const PlanetSettings& settings)
{
    _settings = settings;
}

float Planet::SampleHeight(float x, float y, float z) const
{
    float totalHeight = 0.0f;
    float maskValue = 1.0f;
    bool hasMask = false;

    for (const auto& layer : _noiseLayers)
    {
        if (!layer.IsEnabled())
        {
            continue;
        }

        float layerValue = layer.Sample(x, y, z);

        if (layer.UsesMask() && !hasMask)
        {
            // First mask layer defines the mask
            maskValue = layerValue;
            hasMask = true;
        }
        else if (hasMask)
        {
            // Apply mask to subsequent layers
            totalHeight += layerValue * maskValue;
        }
        else
        {
            totalHeight += layerValue;
        }
    }

    return std::clamp(totalHeight, 0.0f, 1.0f);
}

float Planet::GetRadiusAt(float x, float y, float z) const
{
    float height = SampleHeight(x, y, z);
    float displacement = height * _settings.heightScale;
    return _settings.radius + displacement;
}

void Planet::AddNoiseLayer(const NoiseLayerSettings& settings)
{
    _noiseLayers.emplace_back(settings);
}

void Planet::ClearNoiseLayers()
{
    _noiseLayers.clear();
}

void Planet::Reseed(uint32_t seed)
{
    _settings.seed = seed;

    for (size_t i = 0; i < _noiseLayers.size(); ++i)
    {
        auto& layer = _noiseLayers[i];
        auto settings = layer.GetSettings();
        settings.seed = seed + static_cast<uint32_t>(i);
        layer.Configure(settings);
    }
}

} // namespace planets::core
