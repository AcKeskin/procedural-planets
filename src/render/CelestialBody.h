#pragma once

#include "BiomePalette.h"
#include <cstdint>
#include <string>

namespace planets::render
{

class ComputeShader; // Forward declaration

// Abstract base for all celestial body types (planets, moons, asteroids, etc.)
// Each body type owns its shader set, biome palette, and rendering contract
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

    // Shader paths — body type declares which shaders to use
    virtual std::string GetHeightShaderPath() const = 0;
    virtual std::string GetShadingShaderPath() const = 0;
    virtual std::string GetVertexShaderPath() const = 0;
    virtual std::string GetFragmentShaderPath() const = 0;

    // Uniform binding — body type configures its own compute shaders
    virtual void SetShapeUniforms(ComputeShader& shader, uint32_t seed) const = 0;
    virtual void SetShadingUniforms(ComputeShader& shader, uint32_t seed) const = 0;

    // Data-driven biome palette
    virtual BiomePalette LoadBiomePalette() const = 0;

    // Body type metadata
    virtual std::string GetTypeName() const = 0;
    virtual bool HasSolidSurface() const = 0;
    virtual bool HasAtmosphere() const = 0;

protected:
    float _radius = 10.0f;
    float _seaLevel = 0.4f;
    float _atmosphereHeight = 0.08f; // Fraction of radius
};

} // namespace planets::render
