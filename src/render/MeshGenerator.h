#pragma once

#include "Mesh.h"
#include <map>
#include <vector>

namespace planets::core {
class Planet;
}

namespace planets::render {

class MeshGenerator
{
public:
    // Generate unit icosphere with given subdivisions
    static MeshData GenerateIcosphere(int subdivisions);

    // Generate planet mesh with terrain displacement (CPU)
    static MeshData GeneratePlanetMesh(const planets::core::Planet& planet, int subdivisions);

    // Generate planet mesh with pre-computed heights (GPU)
    static MeshData GeneratePlanetMesh(
        int subdivisions,
        float baseRadius,
        const std::vector<float>& heights);

    // Extract vertex positions from icosphere (for GPU compute input)
    static std::vector<glm::vec3> GetIcosphereVertices(int subdivisions);

private:
    static void Subdivide(MeshData& mesh);
    static uint32_t GetMidpoint(
        MeshData& mesh,
        std::map<uint64_t, uint32_t>& cache,
        uint32_t i1,
        uint32_t i2);

    static glm::vec2 SphericalUV(const glm::vec3& position);
};

} // namespace planets::render
