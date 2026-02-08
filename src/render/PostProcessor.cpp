#include "PostProcessor.h"
#include <GL/gl3w.h>
#include <iostream>

namespace planets::render
{

PostProcessor::PostProcessor()
    : _vao(0)
{
}

PostProcessor::~PostProcessor()
{
    Shutdown();
}

bool PostProcessor::Initialize()
{
    if (_vao != 0)
    {
        std::cerr << "PostProcessor already initialized" << std::endl;
        return false;
    }

    // Create empty VAO (required for core profile even without vertex data)
    // Vertex positions generated procedurally in vertex shader using gl_VertexID
    glGenVertexArrays(1, &_vao);

    if (_vao == 0)
    {
        std::cerr << "Failed to create VAO for PostProcessor" << std::endl;
        return false;
    }

    return true;
}

void PostProcessor::RenderQuad() const
{
    if (_vao == 0)
    {
        std::cerr << "PostProcessor not initialized" << std::endl;
        return;
    }

    // Bind empty VAO and draw fullscreen triangle
    // Vertex shader generates positions from gl_VertexID:
    // Vertex 0: (-1, -1) bottom-left
    // Vertex 1: ( 3, -1) far right
    // Vertex 2: (-1,  3) far top
    // Triangle covers entire screen with minimal overdraw
    glBindVertexArray(_vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
}

void PostProcessor::Shutdown()
{
    if (_vao != 0)
    {
        glDeleteVertexArrays(1, &_vao);
        _vao = 0;
    }
}

} // namespace planets::render
