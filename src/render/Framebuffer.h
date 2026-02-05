#pragma once

#include <cstdint>

namespace planets::render {

// Framebuffer with color and depth texture attachments for post-processing
class Framebuffer
{
public:
    Framebuffer();
    ~Framebuffer();

    Framebuffer(const Framebuffer&) = delete;
    Framebuffer& operator=(const Framebuffer&) = delete;

    Framebuffer(Framebuffer&& other) noexcept;
    Framebuffer& operator=(Framebuffer&& other) noexcept;

    // Create framebuffer with specified dimensions
    bool Create(uint32_t width, uint32_t height);

    // Recreate attachments with new dimensions (invalidates previous texture handles)
    bool Resize(uint32_t width, uint32_t height);

    void Bind() const;
    void Unbind() const;

    uint32_t GetColorTexture() const { return _colorTexture; }
    uint32_t GetDepthTexture() const { return _depthTexture; }
    uint32_t GetWidth() const { return _width; }
    uint32_t GetHeight() const { return _height; }

private:
    void Cleanup();

    uint32_t _fbo;
    uint32_t _colorTexture;
    uint32_t _depthTexture;
    uint32_t _width;
    uint32_t _height;
};

} // namespace planets::render
