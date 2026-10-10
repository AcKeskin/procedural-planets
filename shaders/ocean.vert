#version 450 core

// Ocean surface drawn over the terrain LOD patches: the same GPU-resident patch buffers the planet
// pass reads (bindings match render/lod/PatchBindings.h), with every vertex placed at sea level.
// The ocean gets the terrain's tessellation and LOD, and its depth comes from the drawn heights.

#include "ocean_depth.glsl"

layout(std430, binding = 0) readonly buffer PatchPosition { float position[]; }; // float3 per vertex
layout(std430, binding = 1) readonly buffer PatchHeight   { float height[]; };   // float  per vertex

uniform mat4 uView;
uniform mat4 uProjection;
uniform float uMorphFactor; // set per patch by PlanetQuadTree::Render, same as the planet pass
uniform int uResolution;    // set per patch by SpherePatch::Render
uniform float uPlanetRadius;
uniform float uSeaLevel;    // radial multiplier shared with terrain shading and the atmosphere

out vec3 fragWorldPos;
out vec3 fragNormal;  // unit sphere direction
out float fragDepth;  // water depth in planet-radius units (see ocean_depth.glsl)

vec3 readPosition(int id) { return vec3(position[id * 3], position[id * 3 + 1], position[id * 3 + 2]); }
float readHeight(int id)  { return height[id]; }

// Height on the 2x-decimated parent grid: a copy of planet_earth.vert's morphHeightAt, so the
// ocean sees the same geomorphed coastline as the terrain at every LOD transition.
float morphHeightAt(int i, int j)
{
    bool oddI = (i & 1) != 0;
    bool oddJ = (j & 1) != 0;
    int i0 = oddI ? i - 1 : i;
    int i1 = oddI ? min(i + 1, uResolution - 1) : i;
    int j0 = oddJ ? j - 1 : j;
    int j1 = oddJ ? min(j + 1, uResolution - 1) : j;
    if (oddI && oddJ)
        return 0.25 * (readHeight(j0 * uResolution + i0) + readHeight(j0 * uResolution + i1)
                     + readHeight(j1 * uResolution + i0) + readHeight(j1 * uResolution + i1));
    if (oddI)
        return 0.5 * (readHeight(j * uResolution + i0) + readHeight(j * uResolution + i1));
    if (oddJ)
        return 0.5 * (readHeight(j0 * uResolution + i) + readHeight(j1 * uResolution + i));
    return readHeight(j * uResolution + i);
}

// Interior vertex a skirt vertex duplicates (same order as planet_earth.vert). The ocean needs no
// skirt, so skirt vertices sit on their source vertex and their triangles collapse.
int skirtSourceId(int skirtLocal)
{
    int edge = skirtLocal / uResolution;
    int k = skirtLocal - edge * uResolution;
    if (edge == 0) return k;
    if (edge == 1) return (uResolution - 1) * uResolution + k;
    if (edge == 2) return k * uResolution;
    return k * uResolution + (uResolution - 1);
}

void main()
{
    int interior = uResolution * uResolution;
    int srcId = gl_VertexID >= interior ? skirtSourceId(gl_VertexID - interior) : gl_VertexID;

    vec3 unit = readPosition(srcId);
    float morphH = morphHeightAt(srcId % uResolution, srcId / uResolution);

    fragWorldPos = unit * uPlanetRadius * uSeaLevel;
    fragNormal = unit;
    fragDepth = oceanDepth(uSeaLevel, readHeight(srcId), morphH, uMorphFactor);

    gl_Position = uProjection * uView * vec4(fragWorldPos, 1.0);
}
