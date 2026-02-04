#version 450 core

// Fullscreen triangle vertex shader
// Generates positions procedurally from gl_VertexID without vertex buffers
// Outputs UV coordinates for texture sampling

out vec2 vTexCoord;

void main()
{
    // Generate fullscreen triangle covering NDC space
    // Vertex 0: (-1, -1) -> UV (0, 0)
    // Vertex 1: ( 3, -1) -> UV (2, 0)
    // Vertex 2: (-1,  3) -> UV (0, 2)
    float x = float((gl_VertexID & 1) << 2) - 1.0;
    float y = float((gl_VertexID & 2) << 1) - 1.0;

    vTexCoord = vec2((x + 1.0) * 0.5, (y + 1.0) * 0.5);
    gl_Position = vec4(x, y, 0.0, 1.0);
}
