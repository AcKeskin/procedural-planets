#pragma once

#include <glm/glm.hpp>

namespace planets::render {

class Renderer
{
public:
    Renderer();
    ~Renderer();

    bool Initialize();
    void Shutdown();

    void BeginFrame();
    void EndFrame();

    void SetClearColor(const glm::vec4& color);

private:
    glm::vec4 _clearColor;
};

} // namespace planets::render
