#pragma once

#include "CelestialBody.h"
#include "settings/TerrainSettings.h"
#include "settings/SurfaceSettings.h"
#include <cstdint>

namespace planets::render
{

class ComputeShader;

// Earth-like planet with continental noise, mountain ridges, and ocean depth
// Encapsulates terrain generation parameters for realistic Earth-style surfaces
class Earth : public CelestialBody
{
public:
    Earth();
    ~Earth() override = default;

    // Access to terrain configuration for GUI editing
    EarthTerrainSettings& GetTerrainSettings() { return _terrainSettings; }
    const EarthTerrainSettings& GetTerrainSettings() const { return _terrainSettings; }

    EarthShadingSettings& GetShadingSettings() { return _shadingSettings; }
    const EarthShadingSettings& GetShadingSettings() const { return _shadingSettings; }

    BiomeSettings& GetBiomeSettings() { return _biomeSettings; }
    const BiomeSettings& GetBiomeSettings() const { return _biomeSettings; }

    EarthColors& GetColors() { return _colors; }
    const EarthColors& GetColors() const { return _colors; }

    // Shader paths
    std::string GetHeightShaderPath() const override;
    std::string GetShadingShaderPath() const override;
    std::string GetErosionShaderPath() const override;
    std::string GetVertexShaderPath() const override;
    std::string GetFragmentShaderPath() const override;

    // Uniform binding
    void SetShapeUniforms(ComputeShader& shader, uint32_t seed) const override;
    void SetShadingUniforms(ComputeShader& shader, uint32_t seed) const override;
    void SetRenderUniforms(Shader& shader) const override;

    // Erosion
    bool SupportsErosion() const override;
    void SetErosionUniforms(ComputeShader& shader, size_t vertexCount, int gridResolution) const override;
    int GetErosionIterations() const override;

    // Biome palette
    BiomePalette LoadBiomePalette() const override;

    // Metadata
    std::string GetTypeName() const override { return "Earth"; }
    bool HasSolidSurface() const override { return true; }
    bool HasAtmosphere() const override { return true; }
    float GetHeightScale() const override { return _terrainSettings.heightScale; }

private:
    EarthTerrainSettings _terrainSettings;
    EarthShadingSettings _shadingSettings;
    BiomeSettings _biomeSettings;
    EarthColors _colors;
};

} // namespace planets::render
