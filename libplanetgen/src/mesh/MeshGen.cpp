#include "MeshGen.h"

#include <algorithm>
#include <cmath>

namespace planetgen
{

MeshData MakeUvSphere(uint32_t rings, uint32_t segments)
{
    rings    = std::max<uint32_t>(rings, 2u);
    segments = std::max<uint32_t>(segments, 3u);

    MeshData mesh;
    const uint32_t rows = rings + 1;
    const uint32_t cols = segments + 1;
    mesh.positions.reserve(static_cast<size_t>(rows) * cols * 3);

    constexpr float Pi = 3.14159265358979323846f;

    for (uint32_t r = 0; r < rows; ++r)
    {
        const float v = static_cast<float>(r) / static_cast<float>(rings);
        const float phi = v * Pi;            // 0..pi (pole to pole)
        const float y = std::cos(phi);
        const float ringRadius = std::sin(phi);

        for (uint32_t c = 0; c < cols; ++c)
        {
            const float u = static_cast<float>(c) / static_cast<float>(segments);
            const float theta = u * 2.0f * Pi; // 0..2pi
            mesh.positions.push_back(ringRadius * std::cos(theta));
            mesh.positions.push_back(y);
            mesh.positions.push_back(ringRadius * std::sin(theta));
        }
    }

    mesh.indices.reserve(static_cast<size_t>(rings) * segments * 6);
    for (uint32_t r = 0; r < rings; ++r)
    {
        for (uint32_t c = 0; c < segments; ++c)
        {
            const uint32_t i0 = r * cols + c;
            const uint32_t i1 = i0 + 1;
            const uint32_t i2 = i0 + cols;
            const uint32_t i3 = i2 + 1;

            // Two triangles per quad.
            mesh.indices.push_back(i0);
            mesh.indices.push_back(i2);
            mesh.indices.push_back(i1);

            mesh.indices.push_back(i1);
            mesh.indices.push_back(i2);
            mesh.indices.push_back(i3);
        }
    }

    return mesh;
}

} // namespace planetgen
