#include "PatchLodSystem.h"
#include <iostream>
#include <cmath>

namespace planets::render::lod {

// Icosahedron vertices (golden ratio based)
static const float PHI = (1.0f + std::sqrt(5.0f)) / 2.0f;

static const glm::vec3 IcosahedronVertices[12] = {
    glm::normalize(glm::vec3(-1,  PHI,  0)),
    glm::normalize(glm::vec3( 1,  PHI,  0)),
    glm::normalize(glm::vec3(-1, -PHI,  0)),
    glm::normalize(glm::vec3( 1, -PHI,  0)),
    glm::normalize(glm::vec3( 0, -1,  PHI)),
    glm::normalize(glm::vec3( 0,  1,  PHI)),
    glm::normalize(glm::vec3( 0, -1, -PHI)),
    glm::normalize(glm::vec3( 0,  1, -PHI)),
    glm::normalize(glm::vec3( PHI,  0, -1)),
    glm::normalize(glm::vec3( PHI,  0,  1)),
    glm::normalize(glm::vec3(-PHI,  0, -1)),
    glm::normalize(glm::vec3(-PHI,  0,  1))
};

// Icosahedron faces (20 triangles)
static const int IcosahedronFaces[20][3] = {
    // 5 faces around point 0
    {0, 11, 5}, {0, 5, 1}, {0, 1, 7}, {0, 7, 10}, {0, 10, 11},
    // 5 adjacent faces
    {1, 5, 9}, {5, 11, 4}, {11, 10, 2}, {10, 7, 6}, {7, 1, 8},
    // 5 faces around point 3
    {3, 9, 4}, {3, 4, 2}, {3, 2, 6}, {3, 6, 8}, {3, 8, 9},
    // 5 adjacent faces
    {4, 9, 5}, {2, 4, 11}, {6, 2, 10}, {8, 6, 7}, {9, 8, 1}
};

PatchLodSystem::PatchLodSystem() = default;
PatchLodSystem::~PatchLodSystem() = default;

void PatchLodSystem::Initialize(
    float planetRadius,
    int patchSubdivisions,
    TerrainGenerator& terrainGen,
    const EarthTerrainSettings& terrainSettings,
    const EarthShadingSettings& shadingSettings,
    uint32_t seed)
{
    _planetRadius = planetRadius;
    _patches.clear();

    // Create patches from icosahedron
    CreateIcosahedronPatches(patchSubdivisions);

    std::cout << "[PatchLodSystem] Created " << _patches.size() << " patches" << std::endl;

    // Generate terrain for each patch
    int patchIndex = 0;
    for (auto& patch : _patches)
    {
        GenerateTerrainForPatch(patch, terrainGen, terrainSettings, shadingSettings, seed);
        ++patchIndex;

        if (patchIndex % 10 == 0)
        {
            std::cout << "[PatchLodSystem] Generated " << patchIndex
                      << "/" << _patches.size() << " patches" << std::endl;
        }
    }

    _selectedLods.resize(_patches.size(), SpherePatch::MaxLodLevels - 1);
    _visibility.resize(_patches.size(), true);

    std::cout << "[PatchLodSystem] Terrain generation complete" << std::endl;
}

void PatchLodSystem::CreateIcosahedronPatches(int subdivisions)
{
    // Start with base icosahedron faces
    struct Face { glm::vec3 v0, v1, v2; };
    std::vector<Face> faces;

    for (const auto& f : IcosahedronFaces)
    {
        faces.push_back({
            IcosahedronVertices[f[0]],
            IcosahedronVertices[f[1]],
            IcosahedronVertices[f[2]]
        });
    }

    // Subdivide faces
    for (int s = 0; s < subdivisions; ++s)
    {
        std::vector<Face> newFaces;
        newFaces.reserve(faces.size() * 4);

        for (const auto& face : faces)
        {
            // Calculate midpoints
            glm::vec3 m01 = glm::normalize((face.v0 + face.v1) * 0.5f);
            glm::vec3 m12 = glm::normalize((face.v1 + face.v2) * 0.5f);
            glm::vec3 m20 = glm::normalize((face.v2 + face.v0) * 0.5f);

            // Create 4 new faces
            newFaces.push_back({ face.v0, m01, m20 });
            newFaces.push_back({ m01, face.v1, m12 });
            newFaces.push_back({ m20, m12, face.v2 });
            newFaces.push_back({ m01, m12, m20 });
        }

        faces = std::move(newFaces);
    }

    // Create patches from faces
    _patches.reserve(faces.size());
    for (const auto& face : faces)
    {
        SpherePatch patch;
        patch.Initialize(face.v0, face.v1, face.v2, _planetRadius);
        _patches.push_back(std::move(patch));
    }
}

void PatchLodSystem::GenerateTerrainForPatch(
    SpherePatch& patch,
    TerrainGenerator& terrainGen,
    const EarthTerrainSettings& terrainSettings,
    const EarthShadingSettings& shadingSettings,
    uint32_t seed)
{
    // Generate terrain for all LOD levels
    for (int lod = 0; lod < SpherePatch::MaxLodLevels; ++lod)
    {
        const auto& vertices = patch.GetVertices(lod);

        if (terrainGen.IsReady())
        {
            auto heights = terrainGen.GenerateHeights(vertices, seed, terrainSettings);

            // Generate shading data if available
            std::vector<glm::vec4> shadingData;
            if (terrainGen.IsShadingReady())
            {
                shadingData = terrainGen.GenerateShadingData(vertices, seed, shadingSettings);
            }
            else
            {
                shadingData.resize(vertices.size(), glm::vec4(0.0f));
            }

            patch.GenerateLod(lod, heights, shadingData);
        }
        else
        {
            // Fallback: all heights = 1.0 (unit sphere), no shading
            std::vector<float> heights(vertices.size(), 1.0f);
            std::vector<glm::vec4> shadingData(vertices.size(), glm::vec4(0.0f));
            patch.GenerateLod(lod, heights, shadingData);
        }
    }
}

void PatchLodSystem::UpdateLods(const glm::vec3& cameraPos, const glm::mat4& viewProjection)
{
    _currentFrustum = Frustum::FromViewProjection(viewProjection);
    _visibleCount = 0;

    for (size_t i = 0; i < _patches.size(); ++i)
    {
        // Check visibility
        _visibility[i] = _patches[i].IsVisible(_currentFrustum);

        if (_visibility[i])
        {
            // Select LOD
            _selectedLods[i] = _patches[i].SelectLod(cameraPos);
            ++_visibleCount;
        }
    }
}

void PatchLodSystem::Render(const Shader& shader) const
{
    for (size_t i = 0; i < _patches.size(); ++i)
    {
        if (_visibility[i])
        {
            _patches[i].Render(_selectedLods[i]);
        }
    }
}

int PatchLodSystem::GetTotalVertexCount() const
{
    int total = 0;
    for (size_t i = 0; i < _patches.size(); ++i)
    {
        if (_visibility[i])
        {
            total += _patches[i].GetVertexCount(_selectedLods[i]);
        }
    }
    return total;
}

} // namespace planets::render::lod
