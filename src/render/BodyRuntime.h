#pragma once

// THROWAWAY SHIM — deleted when GenerationPipeline lands.
// Bridges a BodyConfig + resolved palette to the interface call sites currently expect from
// CelestialBody. Exposes exactly the surface used by GenerationScheduler, TerrainGenerator,
// SurfacePanel, and Application's frame loop.

#include "BiomePalette.h"
#include "ComputeShader.h"
#include "Shader.h"
#include "GpuBuffer.h"
#include <planetgen/planetgen.h>
#include "model/BodyConfig.h"
#include "model/PaletteRegistry.h"
#include <cstddef>
#include <cstdint>
#include <string>

namespace planets::render
{

// Adapts a BodyConfig to the runtime call contract without subclassing.
class BodyRuntime
{
public:
    // Construct from a loaded config; palette resolved from the supplied registry.
    BodyRuntime(planetgen::BodyConfig config, const planetgen::PaletteRegistry& registry);
    ~BodyRuntime() = default;

    BodyRuntime(const BodyRuntime&) = delete;
    BodyRuntime& operator=(const BodyRuntime&) = delete;

    // Direct config/palette access for GUI editing
    planetgen::BodyConfig&       Config()       { return _config; }
    const planetgen::BodyConfig& Config() const { return _config; }

    // Shader paths
    std::string GetHeightShaderPath()   const { return _config.shaderPaths.heightShaderPath;   }
    std::string GetShadingShaderPath()  const { return _config.shaderPaths.shadingShaderPath;  }
    std::string GetErosionShaderPath()  const { return _config.shaderPaths.erosionShaderPath;  }
    std::string GetVertexShaderPath()   const { return _config.shaderPaths.vertexShaderPath;   }
    std::string GetFragmentShaderPath() const { return _config.shaderPaths.fragmentShaderPath; }

    // Continent mask — set before calling SetShapeUniforms so it binds the sampler
    void SetContinentMaskTexture(uint32_t textureId) { _continentMaskTexId = textureId; }
    uint32_t GetContinentMaskTexture() const { return _continentMaskTexId; }

    // Uniform binding — uploads a std140 UBO block for each compute stage.
    // vertexCount is packed into the UBO's numVertices field.
    void SetShapeUniforms(ComputeShader& shader, uint32_t seed, uint32_t vertexCount) const;
    void SetShadingUniforms(ComputeShader& shader, uint32_t seed, uint32_t vertexCount) const;
    void SetRenderUniforms(Shader& shader) const;

    // Erosion
    bool SupportsErosion() const { return _config.erosion.enabled; }
    void SetErosionUniforms(ComputeShader& shader, uint32_t vertexCount, int gridResolution) const;
    int  GetErosionIterations() const { return _config.erosion.enabled ? _config.erosion.iterations : 0; }

    // Metadata
    float       GetRadius()          const { return _config.metadata.radius;           }
    float       GetSeaLevel()        const { return _config.metadata.seaLevel;         }
    float       GetAtmosphereRadius() const { return _config.metadata.radius * (1.0f + _config.metadata.atmosphereHeight); }
    bool        HasAtmosphere()      const { return _config.metadata.hasAtmosphere;    }
    bool        HasSolidSurface()    const { return _config.metadata.hasSolidSurface;  }
    std::string GetTypeName()        const { return _config.metadata.name;             }
    float       GetHeightScale()     const { return _config.shape.heightScale;         }

    // Palette
    BiomePalette LoadBiomePalette() const;

private:
    planetgen::BodyConfig       _config;
    const planetgen::PaletteRegistry& _registry;
    uint32_t                    _continentMaskTexId = 0;

    // Cached BiomePalette converted from PaletteRegistry on construction
    mutable BiomePalette _biomePalette;
    mutable bool         _paletteLoaded = false;

    void EnsurePalette() const;
};

} // namespace planets::render
