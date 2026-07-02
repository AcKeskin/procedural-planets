#include "SharedPatchTopology.h"
#include <GL/gl3w.h>
#include <vector>

namespace planets::render::lod
{

SharedPatchTopology::~SharedPatchTopology()
{
    Cleanup();
}

SharedPatchTopology::SharedPatchTopology(SharedPatchTopology&& other) noexcept
    : _vao(other._vao)
    , _ebo(other._ebo)
    , _indexCount(other._indexCount)
    , _resolution(other._resolution)
{
    other._vao = 0;
    other._ebo = 0;
    other._indexCount = 0;
    other._resolution = 0;
}

SharedPatchTopology& SharedPatchTopology::operator=(SharedPatchTopology&& other) noexcept
{
    if (this != &other)
    {
        Cleanup();
        _vao = other._vao;
        _ebo = other._ebo;
        _indexCount = other._indexCount;
        _resolution = other._resolution;
        other._vao = 0;
        other._ebo = 0;
        other._indexCount = 0;
        other._resolution = 0;
    }
    return *this;
}

void SharedPatchTopology::Cleanup()
{
    if (_ebo)
        glDeleteBuffers(1, &_ebo);
    if (_vao)
        glDeleteVertexArrays(1, &_vao);
    _vao = 0;
    _ebo = 0;
}

void SharedPatchTopology::Build(int resolution)
{
    Cleanup();
    _resolution = resolution;
    const int res = resolution;
    const uint32_t interior = static_cast<uint32_t>(res) * res;

    std::vector<uint32_t> indices;
    indices.reserve((res - 1) * (res - 1) * 6 + 4 * (res - 1) * 6);

    // Interior grid triangles — mirrors SpherePatch::GenerateGridIndices.
    for (int j = 0; j < res - 1; ++j)
    {
        for (int i = 0; i < res - 1; ++i)
        {
            uint32_t topLeft = static_cast<uint32_t>(j) * res + i;
            uint32_t topRight = topLeft + 1;
            uint32_t bottomLeft = static_cast<uint32_t>(j + 1) * res + i;
            uint32_t bottomRight = bottomLeft + 1;

            indices.push_back(topLeft);
            indices.push_back(bottomLeft);
            indices.push_back(topRight);

            indices.push_back(topRight);
            indices.push_back(bottomLeft);
            indices.push_back(bottomRight);
        }
    }

    // Skirt ring — one skirt vertex per edge vertex, edge order bottom/top/left/right, each
    // block of `res` skirt ids sitting at [interior + edge*res, ...). Triangle wiring mirrors
    // SpherePatch::AppendSkirtGeometry (edge0,skirt0,edge1) + (edge1,skirt0,skirt1). The source
    // interior id for a skirt vertex is baked here and re-derived identically in the VS.
    auto interiorEdgeId = [res](int edge, int k) -> uint32_t {
        switch (edge)
        {
        case 0: return static_cast<uint32_t>(k);                              // bottom: row 0
        case 1: return static_cast<uint32_t>(res - 1) * res + k;              // top: row res-1
        case 2: return static_cast<uint32_t>(k) * res;                        // left: col 0
        default: return static_cast<uint32_t>(k) * res + (res - 1);           // right: col res-1
        }
    };

    for (int edge = 0; edge < 4; ++edge)
    {
        const uint32_t skirtBase = interior + static_cast<uint32_t>(edge) * res;
        for (int k = 0; k < res - 1; ++k)
        {
            uint32_t edge0 = interiorEdgeId(edge, k);
            uint32_t edge1 = interiorEdgeId(edge, k + 1);
            uint32_t skirt0 = skirtBase + k;
            uint32_t skirt1 = skirt0 + 1;

            indices.push_back(edge0);
            indices.push_back(skirt0);
            indices.push_back(edge1);

            indices.push_back(edge1);
            indices.push_back(skirt0);
            indices.push_back(skirt1);
        }
    }

    _indexCount = static_cast<uint32_t>(indices.size());

    glGenVertexArrays(1, &_vao);
    glBindVertexArray(_vao);

    glGenBuffers(1, &_ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t), indices.data(), GL_STATIC_DRAW);

    // No vertex attributes: the VS sources all per-vertex data from SSBOs by gl_VertexID.
    glBindVertexArray(0);
}

void SharedPatchTopology::Draw() const
{
    if (_vao == 0 || _indexCount == 0)
        return;

    glBindVertexArray(_vao);
    glDrawElements(GL_TRIANGLES, _indexCount, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

} // namespace planets::render::lod
