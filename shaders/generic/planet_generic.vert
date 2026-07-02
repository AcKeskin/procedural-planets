#version 450 core

// GPU-resident patch draw (generic body): per-vertex data from the compute-written SSBOs, indexed
// by gl_VertexID. No vertex attributes, no geomorph (generic bodies never had morph-to-parent).
// Binding points must match render/lod/PatchBindings.h.
layout(std430, binding = 0) readonly buffer PatchPosition { float position[]; }; // float3 per vertex
layout(std430, binding = 1) readonly buffer PatchHeight   { float height[]; };   // float  per vertex
layout(std430, binding = 2) readonly buffer PatchNormal   { float normalData[]; }; // float3 per vertex
layout(std430, binding = 3) readonly buffer PatchShading   { vec4 shading[]; };   // float4 per vertex

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;

uniform int uResolution;
uniform float uPlanetRadius;
uniform float uSkirtDepth;

out vec3 vNormal;
out vec3 vWorldPos;
out float vHeight;
out vec4 vShadingData;

vec3 readPosition(int id) { return vec3(position[id * 3], position[id * 3 + 1], position[id * 3 + 2]); }
vec3 readNormal(int id)   { return vec3(normalData[id * 3], normalData[id * 3 + 1], normalData[id * 3 + 2]); }

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
    bool isSkirt = gl_VertexID >= interior;
    int srcId = isSkirt ? skirtSourceId(gl_VertexID - interior) : gl_VertexID;

    vec3 unit = readPosition(srcId);
    float h = height[srcId];
    vec3 pos = unit * uPlanetRadius * h;

    if (isSkirt)
        pos -= normalize(pos) * uSkirtDepth;

    vec4 worldPos = uModel * vec4(pos, 1.0);
    vWorldPos = worldPos.xyz;
    vNormal = mat3(transpose(inverse(uModel))) * readNormal(srcId);
    vHeight = h;
    vShadingData = shading[srcId];

    gl_Position = uProjection * uView * worldPos;
}
