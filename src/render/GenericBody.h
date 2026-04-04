#pragma once

#include "CelestialBody.h"
#include <string>
#include <unordered_map>
#include <memory>

namespace planets::render
{

// Configurable noise layer for generic terrain generation
struct GenericNoiseLayer
{
    float scale = 1.0f;
    int octaves = 4;
    float persistence = 0.5f;
    float lacunarity = 2.0f;
    float strength = 1.0f;
};

// Complete configuration for a data-driven celestial body
struct GenericBodyConfig
{
    std::string name = "Unknown";
    float radius = 10.0f;
    float seaLevel = 0.0f;
    float atmosphereHeight = 0.0f;
    bool hasSolidSurface = true;
    bool hasAtmosphere = false;

    // Shader paths (defaults to generic shaders)
    std::string heightShaderPath = "shaders/compute/height_earth.comp";
    std::string shadingShaderPath = "shaders/compute/shading_generic.comp";
    std::string vertexShaderPath = "shaders/generic/planet_generic.vert";
    std::string fragmentShaderPath = "shaders/generic/planet_generic.frag";
    std::string palettePath;

    // Terrain noise parameters
    GenericNoiseLayer continentNoise;
    GenericNoiseLayer mountainNoise;
    GenericNoiseLayer maskNoise;
    float heightScale = 0.04f;
    float oceanDepthMultiplier = 3.0f;
    float oceanFloorDepth = 0.02f;
    float mountainBlend = 0.5f;

    // Shading noise parameters
    float detailNoiseScale = 2.0f;
    float smallNoiseScale = 15.0f;
    int detailNoiseOctaves = 3;
    int smallNoiseOctaves = 5;
    float warpStrength = 0.3f;
};

// Data-driven celestial body loaded from JSON config
// Supports arbitrary palettes and configurable noise parameters
class GenericBody : public CelestialBody
{
public:
    GenericBody() = default;
    ~GenericBody() override = default;

    static std::unique_ptr<GenericBody> LoadFromJson(const std::string& configPath);

    GenericBodyConfig& GetConfig() { return _config; }
    const GenericBodyConfig& GetConfig() const { return _config; }

    // Shader paths
    std::string GetHeightShaderPath() const override;
    std::string GetShadingShaderPath() const override;
    std::string GetVertexShaderPath() const override;
    std::string GetFragmentShaderPath() const override;

    // Uniform binding
    void SetShapeUniforms(ComputeShader& shader, uint32_t seed) const override;
    void SetShadingUniforms(ComputeShader& shader, uint32_t seed) const override;

    // Palette
    BiomePalette LoadBiomePalette() const override;

    // Metadata
    std::string GetTypeName() const override { return _config.name; }
    bool HasSolidSurface() const override { return _config.hasSolidSurface; }
    bool HasAtmosphere() const override { return _config.hasAtmosphere; }

private:
    GenericBodyConfig _config;
};

} // namespace planets::render
