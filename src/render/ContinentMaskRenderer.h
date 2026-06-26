#pragma once

// Generates and owns the 3D continent mask texture.
// Runs continent_growth.comp via the app-side ComputeShader to avoid
// adding 3D-texture plumbing to libplanetgen's GlDevice.

#include "ComputeShader.h"
#include <model/BodyConfig.h>
#include <cstdint>

namespace planets::render
{

class ContinentMaskRenderer
{
public:
    ContinentMaskRenderer();
    ~ContinentMaskRenderer();

    ContinentMaskRenderer(const ContinentMaskRenderer&) = delete;
    ContinentMaskRenderer& operator=(const ContinentMaskRenderer&) = delete;

    // Load the growth shader. Call once after the GL context exists.
    bool Initialize();

    // Regenerate the mask from the given config and seed.
    // Safe to call repeatedly; tears down the old texture first.
    void Generate(const planetgen::BodyConfig& config, uint32_t seed);

    // GL texture object — valid after at least one successful Generate().
    uint32_t GetMaskTextureId()  const { return _texture; }
    bool     IsReady()           const { return _shader.IsValid(); }   // shader loaded
    bool     HasTexture()        const { return _texture != 0; }       // texture generated

private:
    void DestroyTexture();

    ComputeShader _shader;
    uint32_t      _texture = 0;  // GL_TEXTURE_3D, RG32F
};

} // namespace planets::render
