#version 450 core

// GPU-resident patch draw: all per-vertex data comes from the compute-written SSBOs, indexed by
// gl_VertexID. No vertex attributes. Position displacement, geomorph, and skirts are computed
// here — the CPU never touches the mesh. Binding points must match render/lod/PatchBindings.h.
layout(std430, binding = 0) readonly buffer PatchPosition { float position[]; }; // float3 per vertex
layout(std430, binding = 1) readonly buffer PatchHeight   { float height[]; };   // float  per vertex
layout(std430, binding = 2) readonly buffer PatchNormal   { float normalData[]; }; // float3 per vertex
layout(std430, binding = 3) readonly buffer PatchShading   { vec4 shading[]; };   // float4 per vertex

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
uniform float uMorphFactor; // 0 = full fine detail, 1 = collapsed onto coarse parent grid

uniform int uResolution;      // vertices per patch edge; interior grid is uResolution^2 vertices
uniform float uPlanetRadius;
uniform float uSkirtDepth;    // world-space inward push for skirt vertices

out vec3 vNormal;
out vec3 vWorldPos;
out float vHeight;
out vec4 vShadingData;

vec3 readPosition(int id) { return vec3(position[id * 3], position[id * 3 + 1], position[id * 3 + 2]); }
vec3 readNormal(int id)   { return vec3(normalData[id * 3], normalData[id * 3 + 1], normalData[id * 3 + 2]); }
float readHeight(int id)  { return height[id]; }

// Morph target height: the height the patch would have on the 2x-decimated parent grid. Even/even
// vertices survive decimation unchanged; odd indices average the surviving even neighbours. Mirror
// of the old CPU SpherePatch::morphHeightAt, computed here so geomorph stays byte-identical.
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

// Interior vertex id a skirt vertex duplicates. Edge order bottom/top/left/right matches
// SharedPatchTopology::interiorEdgeId and the CPU AppendSkirtGeometry.
int skirtSourceId(int skirtLocal)
{
    int edge = skirtLocal / uResolution;
    int k = skirtLocal - edge * uResolution;
    if (edge == 0) return k;                              // bottom: row 0
    if (edge == 1) return (uResolution - 1) * uResolution + k; // top: row res-1
    if (edge == 2) return k * uResolution;               // left: col 0
    return k * uResolution + (uResolution - 1);          // right: col res-1
}

void main()
{
    int interior = uResolution * uResolution;
    bool isSkirt = gl_VertexID >= interior;
    int srcId = isSkirt ? skirtSourceId(gl_VertexID - interior) : gl_VertexID;

    vec3 unit = readPosition(srcId);
    float h = readHeight(srcId);
    vec3 pos = unit * uPlanetRadius * h;

    int gx = srcId % uResolution;
    int gy = srcId / uResolution;
    float morphH = morphHeightAt(gx, gy);
    vec3 morphPos = unit * uPlanetRadius * morphH;

    // Geomorph toward the coarse-grid shape as the patch nears its merge distance.
    vec3 morphed = mix(pos, morphPos, uMorphFactor);

    // Skirt vertices push radially inward so the skirt collapses with the surface, hiding cracks.
    if (isSkirt)
        morphed -= normalize(morphed) * uSkirtDepth;

    vec4 worldPos = uModel * vec4(morphed, 1.0);
    vWorldPos = worldPos.xyz;
    vNormal = mat3(transpose(inverse(uModel))) * readNormal(srcId);
    vHeight = h;
    vShadingData = shading[srcId];

    gl_Position = uProjection * uView * worldPos;
}
