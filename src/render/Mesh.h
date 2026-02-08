#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <cstdint>

namespace planets::render
{

struct Vertex
{
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 uv;
    glm::vec4 shadingData; // biome, detail, fine, warp noise
};

struct MeshData
{
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    void Clear()
    {
        vertices.clear();
        indices.clear();
    }

    void RecalculateNormals();
};

// GPU mesh representation with VAO/VBO/EBO
class Mesh
{
public:
    Mesh();
    ~Mesh();

    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;

    Mesh(Mesh&& other) noexcept;
    Mesh& operator=(Mesh&& other) noexcept;

    void Upload(const MeshData& data);
    void Draw() const;

    uint32_t GetVertexCount() const { return _vertexCount; }
    uint32_t GetIndexCount() const { return _indexCount; }

private:
    void Cleanup();

    uint32_t _vao;
    uint32_t _vbo;
    uint32_t _ebo;
    uint32_t _vertexCount;
    uint32_t _indexCount;
};

} // namespace planets::render
