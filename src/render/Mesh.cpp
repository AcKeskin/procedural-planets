#include "Mesh.h"
#include <GL/gl3w.h>
#include <unordered_map>

namespace planets::render {

void MeshData::RecalculateNormals()
{
    // Reset all normals
    for (auto& v : vertices)
    {
        v.normal = glm::vec3(0.0f);
    }

    // Accumulate face normals
    for (size_t i = 0; i < indices.size(); i += 3)
    {
        uint32_t i0 = indices[i];
        uint32_t i1 = indices[i + 1];
        uint32_t i2 = indices[i + 2];

        glm::vec3 v0 = vertices[i0].position;
        glm::vec3 v1 = vertices[i1].position;
        glm::vec3 v2 = vertices[i2].position;

        glm::vec3 edge1 = v1 - v0;
        glm::vec3 edge2 = v2 - v0;
        glm::vec3 faceNormal = glm::cross(edge1, edge2);

        vertices[i0].normal += faceNormal;
        vertices[i1].normal += faceNormal;
        vertices[i2].normal += faceNormal;
    }

    // Normalize
    for (auto& v : vertices)
    {
        if (glm::length(v.normal) > 0.0001f)
        {
            v.normal = glm::normalize(v.normal);
        }
    }
}

Mesh::Mesh()
    : _vao(0)
    , _vbo(0)
    , _ebo(0)
    , _vertexCount(0)
    , _indexCount(0)
{
}

Mesh::~Mesh()
{
    Cleanup();
}

Mesh::Mesh(Mesh&& other) noexcept
    : _vao(other._vao)
    , _vbo(other._vbo)
    , _ebo(other._ebo)
    , _vertexCount(other._vertexCount)
    , _indexCount(other._indexCount)
{
    other._vao = 0;
    other._vbo = 0;
    other._ebo = 0;
    other._vertexCount = 0;
    other._indexCount = 0;
}

Mesh& Mesh::operator=(Mesh&& other) noexcept
{
    if (this != &other)
    {
        Cleanup();
        _vao = other._vao;
        _vbo = other._vbo;
        _ebo = other._ebo;
        _vertexCount = other._vertexCount;
        _indexCount = other._indexCount;
        other._vao = 0;
        other._vbo = 0;
        other._ebo = 0;
        other._vertexCount = 0;
        other._indexCount = 0;
    }
    return *this;
}

void Mesh::Upload(const MeshData& data)
{
    Cleanup();

    _vertexCount = static_cast<uint32_t>(data.vertices.size());
    _indexCount = static_cast<uint32_t>(data.indices.size());

    if (_vertexCount == 0 || _indexCount == 0)
    {
        return;
    }

    glGenVertexArrays(1, &_vao);
    glGenBuffers(1, &_vbo);
    glGenBuffers(1, &_ebo);

    glBindVertexArray(_vao);

    glBindBuffer(GL_ARRAY_BUFFER, _vbo);
    glBufferData(GL_ARRAY_BUFFER,
        data.vertices.size() * sizeof(Vertex),
        data.vertices.data(),
        GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
        data.indices.size() * sizeof(uint32_t),
        data.indices.data(),
        GL_STATIC_DRAW);

    // Position attribute
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
        reinterpret_cast<void*>(offsetof(Vertex, position)));

    // Normal attribute
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
        reinterpret_cast<void*>(offsetof(Vertex, normal)));

    // UV attribute
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
        reinterpret_cast<void*>(offsetof(Vertex, uv)));

    glBindVertexArray(0);
}

void Mesh::Draw() const
{
    if (_vao == 0 || _indexCount == 0)
    {
        return;
    }

    glBindVertexArray(_vao);
    glDrawElements(GL_TRIANGLES, _indexCount, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

void Mesh::Cleanup()
{
    if (_ebo)
    {
        glDeleteBuffers(1, &_ebo);
        _ebo = 0;
    }
    if (_vbo)
    {
        glDeleteBuffers(1, &_vbo);
        _vbo = 0;
    }
    if (_vao)
    {
        glDeleteVertexArrays(1, &_vao);
        _vao = 0;
    }
    _vertexCount = 0;
    _indexCount = 0;
}

} // namespace planets::render
