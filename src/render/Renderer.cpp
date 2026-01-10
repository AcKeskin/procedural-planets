#include "Renderer.h"
#include <GL/gl3w.h>

namespace planets::render {

Renderer::Renderer()
    : _clearColor(0.1f, 0.1f, 0.2f, 1.0f)
{
}

Renderer::~Renderer()
{
    Shutdown();
}

bool Renderer::Initialize()
{
    glEnable(GL_DEPTH_TEST);
    return true;
}

void Renderer::Shutdown()
{
}

void Renderer::BeginFrame()
{
    glClearColor(_clearColor.r, _clearColor.g, _clearColor.b, _clearColor.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::EndFrame()
{
}

void Renderer::SetClearColor(const glm::vec4& color)
{
    _clearColor = color;
}

} // namespace planets::render
