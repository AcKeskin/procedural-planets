#include "BloomPass.h"
#include "../PostProcessor.h"
#include <GL/gl3w.h>
#include <algorithm>
#include <iostream>

namespace planets::render::effects
{

namespace
{
// Mip 5 of the half-res target is 1/64 of the screen: wide enough for the outer glow.
constexpr int BloomMipLevels = 6;
} // namespace

BloomPass::~BloomPass()
{
    Cleanup();
}

bool BloomPass::Initialize(int width, int height)
{
    if (!_brightShader.LoadFromFiles("shaders/passthrough.vert", "shaders/bloom_bright.frag"))
    {
        std::cerr << "[BloomPass] Failed to load bright-pass shader" << std::endl;
        return false;
    }
    Resize(width, height);
    return _fbo != 0;
}

void BloomPass::Resize(int width, int height)
{
    Cleanup();
    _width = std::max(1, width / 2);
    _height = std::max(1, height / 2);

    glCreateTextures(GL_TEXTURE_2D, 1, &_texture);
    glTextureStorage2D(_texture, BloomMipLevels, GL_RGBA16F, _width, _height);
    glTextureParameteri(_texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
    glTextureParameteri(_texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(_texture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(_texture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glCreateFramebuffers(1, &_fbo);
    glNamedFramebufferTexture(_fbo, GL_COLOR_ATTACHMENT0, _texture, 0);
    if (glCheckNamedFramebufferStatus(_fbo, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        std::cerr << "[BloomPass] Framebuffer incomplete" << std::endl;
        Cleanup();
    }
}

void BloomPass::Render(uint32_t sceneColorTexture, const PostProcessor& fullscreen)
{
    if (_fbo == 0)
        return;

    glBindFramebuffer(GL_FRAMEBUFFER, _fbo);
    glViewport(0, 0, _width, _height);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    _brightShader.Use();
    _brightShader.SetInt("uSceneTexture", 0);
    glBindTextureUnit(0, sceneColorTexture);
    fullscreen.RenderQuad();

    glGenerateTextureMipmap(_texture);
    glEnable(GL_DEPTH_TEST);
}

void BloomPass::Cleanup()
{
    if (_fbo)
        glDeleteFramebuffers(1, &_fbo);
    if (_texture)
        glDeleteTextures(1, &_texture);
    _fbo = 0;
    _texture = 0;
}

} // namespace planets::render::effects
