#include "Framebuffer.h"
#include <GL/gl3w.h>
#include <iostream>

namespace planets::render
{

Framebuffer::Framebuffer()
    : _fbo(0)
    , _colorTexture(0)
    , _depthTexture(0)
    , _width(0)
    , _height(0)
{
}

Framebuffer::~Framebuffer()
{
    Cleanup();
}

Framebuffer::Framebuffer(Framebuffer&& other) noexcept
    : _fbo(other._fbo)
    , _colorTexture(other._colorTexture)
    , _depthTexture(other._depthTexture)
    , _width(other._width)
    , _height(other._height)
{
    other._fbo = 0;
    other._colorTexture = 0;
    other._depthTexture = 0;
    other._width = 0;
    other._height = 0;
}

Framebuffer& Framebuffer::operator=(Framebuffer&& other) noexcept
{
    if (this != &other)
    {
        Cleanup();
        _fbo = other._fbo;
        _colorTexture = other._colorTexture;
        _depthTexture = other._depthTexture;
        _width = other._width;
        _height = other._height;
        other._fbo = 0;
        other._colorTexture = 0;
        other._depthTexture = 0;
        other._width = 0;
        other._height = 0;
    }
    return *this;
}

bool Framebuffer::Create(uint32_t width, uint32_t height)
{
    Cleanup();

    if (width == 0 || height == 0)
    {
        std::cerr << "Framebuffer: invalid dimensions (" << width << "x" << height << ")" << std::endl;
        return false;
    }

    _width = width;
    _height = height;

    // Create framebuffer object
    glCreateFramebuffers(1, &_fbo);

    // Create color texture attachment
    glCreateTextures(GL_TEXTURE_2D, 1, &_colorTexture);
    glTextureStorage2D(_colorTexture, 1, GL_RGBA8, width, height);
    glTextureParameteri(_colorTexture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(_colorTexture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(_colorTexture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(_colorTexture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Create depth texture attachment
    glCreateTextures(GL_TEXTURE_2D, 1, &_depthTexture);
    glTextureStorage2D(_depthTexture, 1, GL_DEPTH_COMPONENT32F, width, height);
    glTextureParameteri(_depthTexture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTextureParameteri(_depthTexture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTextureParameteri(_depthTexture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(_depthTexture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Attach textures to framebuffer
    glNamedFramebufferTexture(_fbo, GL_COLOR_ATTACHMENT0, _colorTexture, 0);
    glNamedFramebufferTexture(_fbo, GL_DEPTH_ATTACHMENT, _depthTexture, 0);

    // Specify draw buffers
    GLenum drawBuffers[] = {GL_COLOR_ATTACHMENT0};
    glNamedFramebufferDrawBuffers(_fbo, 1, drawBuffers);

    // Check completeness
    GLenum status = glCheckNamedFramebufferStatus(_fbo, GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE)
    {
        std::cerr << "Framebuffer: incomplete (status: 0x" << std::hex << status << std::dec << ")" << std::endl;
        Cleanup();
        return false;
    }

    return true;
}

bool Framebuffer::Resize(uint32_t width, uint32_t height)
{
    // Delegate to Create which handles cleanup
    return Create(width, height);
}

void Framebuffer::Bind() const
{
    if (_fbo == 0)
    {
        return;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, _fbo);
    glViewport(0, 0, _width, _height);
}

void Framebuffer::Unbind() const
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Framebuffer::Cleanup()
{
    if (_depthTexture)
    {
        glDeleteTextures(1, &_depthTexture);
        _depthTexture = 0;
    }
    if (_colorTexture)
    {
        glDeleteTextures(1, &_colorTexture);
        _colorTexture = 0;
    }
    if (_fbo)
    {
        glDeleteFramebuffers(1, &_fbo);
        _fbo = 0;
    }
    _width = 0;
    _height = 0;
}

} // namespace planets::render
