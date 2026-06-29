#pragma once

// The app's render-time view of the active body — owns its BodyConfig + resolved
// palette and exposes render metadata + render-uniform binding. Render-time only:
// terrain compute (height/shading/erosion + the continent mask) lives in libplanetgen
// and runs through the public per-patch path, not here.

#include "BiomePalette.h"
#include "Shader.h"
#include <planetgen/planetgen.h>
#include "model/BodyConfig.h"
#include "model/PaletteRegistry.h"
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

    void SetRenderUniforms(Shader& shader) const;

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

    // Cached BiomePalette converted from PaletteRegistry on construction
    mutable BiomePalette _biomePalette;
    mutable bool         _paletteLoaded = false;

    void EnsurePalette() const;
};

} // namespace planets::render
