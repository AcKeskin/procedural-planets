#pragma once

#include "CelestialBody.h"
#include "TerrainGenerator.h"
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

    // Bind shape generation parameters to compute shader
    void SetShapeUniforms(ComputeShader& shader, uint32_t seed) const override;

    // Bind surface shading parameters (future: biome noise, temperature, moisture)
    void SetShadingUniforms(ComputeShader& shader, uint32_t seed) const override;

private:
    EarthTerrainSettings _terrainSettings;
};

} // namespace planets::render
