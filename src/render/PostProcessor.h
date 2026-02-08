#pragma once

#include <cstdint>

namespace planets::render
{

// Manages fullscreen rendering for post-process effects
// Uses efficient fullscreen triangle technique (3 vertices, no buffers needed)
// Caller is responsible for shader binding and uniform setup
class PostProcessor
{
public:
    PostProcessor();
    ~PostProcessor();

    PostProcessor(const PostProcessor&) = delete;
    PostProcessor& operator=(const PostProcessor&) = delete;

    // Initialize empty VAO for fullscreen triangle rendering
    bool Initialize();

    // Render fullscreen triangle (caller must bind shader beforehand)
    void RenderQuad() const;

    // Release OpenGL resources
    void Shutdown();

    bool IsInitialized() const { return _vao != 0; }

private:
    uint32_t _vao;
};

} // namespace planets::render
