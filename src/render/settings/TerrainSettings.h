#pragma once

#include <glm/glm.hpp>
#include <cstdint>

namespace planets::render
{

// Generation parameters (seed, subdivisions, GPU toggle)
struct GenerationConfig
{
    uint32_t seed = 42;
    int subdivisions = 6;
    bool useGpu = true;
};

// LOD system configuration
struct LodConfig
{
    bool enabled = true;
    int patchSubdivisions = 2;
    float planetRadius = 200.0f;

    // Quadtree parameters
    int meshResolution = 32;
    int maxDepth = 8;
    float splitThreshold = 6.0f; // split when dist < threshold*arc*radius; higher = denser geometry up close
    float hysteresis = 1.3f;
    int maxActivePatches = 2000; // generous budget so near-surface patches all reach full depth
                                 // (at 600 the budget ran out before close patches subdivided →
                                 // coarse facets + seam cracks next to detailed neighbours)
    float skirtFraction = 0.02f;
};

// Read-only terrain statistics for display
struct TerrainStats
{
    int patchCount = 0;
    int visiblePatchCount = 0;
    int vertexCount = 0;
    float cpuTimeMs = 0.0f;
    float gpuTimeMs = 0.0f;
    bool gpuAvailable = false;
};

} // namespace planets::render
