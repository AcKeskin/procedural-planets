#pragma once

#include <cstdint>

namespace planets::render::lod
{

// Static topology shared by every patch of a given resolution. The grid connectivity and the
// skirt ring are identical across patches, so the index buffer + VAO are built once and reused;
// only the per-patch SSBO bindings vary at draw time. The VS pulls vertex data from those SSBOs
// by gl_VertexID, so the VAO carries NO vertex attributes — just the element buffer.
//
// Vertex id layout (must match the patch vertex shaders):
//   interior grid : [0, res*res)                 — one per grid vertex, row-major
//   skirt ring    : [res*res, res*res + 4*res)   — one per edge vertex, edge order:
//                     bottom (row 0), top (row res-1), left (col 0), right (col res-1)
// The VS recovers a skirt vertex's source interior id from (gl_VertexID - res*res) using the
// same edge formulas this class bakes into the index buffer.
class SharedPatchTopology
{
public:
    SharedPatchTopology() = default;
    ~SharedPatchTopology();

    SharedPatchTopology(const SharedPatchTopology&) = delete;
    SharedPatchTopology& operator=(const SharedPatchTopology&) = delete;

    SharedPatchTopology(SharedPatchTopology&& other) noexcept;
    SharedPatchTopology& operator=(SharedPatchTopology&& other) noexcept;

    // Build the VAO + shared index buffer for the given grid resolution (vertices per edge).
    // Safe to call once; rebuilding releases the previous GL objects.
    void Build(int resolution);

    // Bind the VAO and issue the indexed draw over interior + skirt triangles.
    void Draw() const;

private:
    void Cleanup();

    uint32_t _vao = 0;
    uint32_t _ebo = 0;
    uint32_t _indexCount = 0;
    int _resolution = 0;
};

} // namespace planets::render::lod
