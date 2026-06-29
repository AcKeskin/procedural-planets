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
    float splitThreshold = 2.0f;
    float hysteresis = 1.3f;
    int maxActivePatches = 400;
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
