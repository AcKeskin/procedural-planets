#version 450 core

// Presents the HDR scene when the atmosphere pass is off: tonemap only.

#include "tonemap.glsl"

in vec2 vTexCoord;
out vec4 fragColor;

uniform sampler2D uSceneTexture;

void main()
{
    fragColor = vec4(tonemap(texture(uSceneTexture, vTexCoord).rgb), 1.0);
}
