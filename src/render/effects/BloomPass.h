#pragma once

#include "../Shader.h"
#include <cstdint>

namespace planets::render
{
class PostProcessor;
}

namespace planets::render::effects
{

// HDR bloom. Keeps only the light above a threshold (the sun, glints) at half resolution and
// blurs it through the mip chain; the final pass reads the mips back (bloom.glsl).
class BloomPass
{
public:
    BloomPass() = default;
    ~BloomPass();

    BloomPass(const BloomPass&) = delete;
    BloomPass& operator=(const BloomPass&) = delete;

    bool Initialize(int width, int height);
    void Resize(int width, int height);

    // Fills the bloom mips from the HDR scene. Leaves the bloom framebuffer bound.
    void Render(uint32_t sceneColorTexture, const PostProcessor& fullscreen);

    uint32_t Texture() const { return _texture; }

private:
    void Cleanup();

    Shader _brightShader;
    uint32_t _fbo = 0;
    uint32_t _texture = 0;
    int _width = 0;
    int _height = 0;
};

} // namespace planets::render::effects
