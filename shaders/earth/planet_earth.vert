#version 450 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUV;
layout(location = 3) in vec4 aShadingData;
layout(location = 4) in vec3 aMorphPosition; // position on the 2x-decimated (coarse) grid

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
uniform float uMorphFactor; // 0 = full fine detail, 1 = collapsed onto coarse parent grid

out vec3 vNormal;
out vec3 vWorldPos;
out float vHeight;
out vec4 vShadingData;

void main()
{
    // Geomorph: blend toward the coarse-grid position as the patch nears its merge distance, so
    // the coarse<->fine LOD swap has nothing to pop. uMorphFactor is per-patch.
    vec3 morphed = mix(aPosition, aMorphPosition, uMorphFactor);

    vec4 worldPos = uModel * vec4(morphed, 1.0);
    vWorldPos = worldPos.xyz;
    vNormal = mat3(transpose(inverse(uModel))) * aNormal;
    vHeight = aUV.x; // Height stored in UV.x
    vShadingData = aShadingData;

    gl_Position = uProjection * uView * worldPos;
}
