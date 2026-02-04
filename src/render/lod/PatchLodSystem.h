#pragma once

#include "SpherePatch.h"
#include "../TerrainGenerator.h"
#include "../Shader.h"
#include <vector>
#include <memory>

namespace planets::render::lod {

// Manages all terrain patches and LOD selection
class PatchLodSystem
{
public:
    PatchLodSystem();
    ~PatchLodSystem();

    PatchLodSystem(const PatchLodSystem&) = delete;
    PatchLodSystem& operator=(const PatchLodSystem&) = delete;

    // Initialize patches for a sphere
    // subdivisions: how many times to subdivide base icosahedron faces (0 = 20 patches, 1 = 80, 2 = 320)
    void Initialize(
        float planetRadius,
        int patchSubdivisions,
        TerrainGenerator& terrainGen,
        const EarthTerrainSettings& settings,
        uint32_t seed);

    // Update LOD levels based on camera position
    void UpdateLods(const glm::vec3& cameraPos, const glm::mat4& viewProjection);

    // Render all visible patches
    void Render(const Shader& shader) const;

    // Get statistics
    int GetPatchCount() const { return static_cast<int>(_patches.size()); }
    int GetVisiblePatchCount() const { return _visibleCount; }
    int GetTotalVertexCount() const;

private:
    void CreateIcosahedronPatches(int subdivisions);
    void GenerateTerrainForPatch(
        SpherePatch& patch,
        TerrainGenerator& terrainGen,
        const EarthTerrainSettings& settings,
        uint32_t seed);

    std::vector<SpherePatch> _patches;
    std::vector<int> _selectedLods;
    std::vector<bool> _visibility;

    float _planetRadius = 1.0f;
    int _visibleCount = 0;
    Frustum _currentFrustum;
};

} // namespace planets::render::lod
