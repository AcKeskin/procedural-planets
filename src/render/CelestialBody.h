#pragma once

#include <cstdint>

namespace planets::render {

class ComputeShader; // Forward declaration

// Abstract base for all celestial body types (planets, moons, asteroids, etc.)
// Defines common physical properties and shader binding interface
class CelestialBody
{
public:
    virtual ~CelestialBody() = default;

    // Physical configuration
    void SetRadius(float radius);
    void SetSeaLevel(float seaLevel);
    void SetAtmosphereHeight(float height);

    float GetRadius() const { return _radius; }
    float GetSeaLevel() const { return _seaLevel; }
    float GetAtmosphereHeight() const { return _atmosphereHeight; }
    float GetAtmosphereRadius() const { return _radius * (1.0f + _atmosphereHeight); }

    // Subclass implements shape generation parameters
    virtual void SetShapeUniforms(ComputeShader& shader, uint32_t seed) const = 0;

    // Subclass implements surface shading parameters (biomes, colors, etc.)
    virtual void SetShadingUniforms(ComputeShader& shader, uint32_t seed) const = 0;

protected:
    float _radius = 10.0f;
    float _seaLevel = 0.4f;
    float _atmosphereHeight = 0.08f; // Fraction of radius
};

} // namespace planets::render
