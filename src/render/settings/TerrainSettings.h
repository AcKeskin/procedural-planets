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

// LOD system configuration. The planet radius is the body's (BodyConfig.metadata.radius).
struct LodConfig
{
    bool enabled = true;
    int patchSubdivisions = 2;

    // Quadtree parameters
    int meshResolution = 32;
    int maxDepth = 8;
    float splitThreshold = 7.0f; // split when dist < threshold*arc*radius; higher = denser geometry
                                 // up close AND splits trigger sooner, so children finish generating
                                 // before they're visually close (hides split-gen latency on approach)
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
    float gpuTerrainMs = 0.0f;
    float gpuOceanMs = 0.0f;
    float gpuEffectsMs = 0.0f;
    bool gpuAvailable = false;
};

} // namespace planets::render
