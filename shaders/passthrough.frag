#version 450 core

// Presents the HDR scene when the atmosphere pass is off: bloom and tonemap only.

#include "tonemap.glsl"
#include "bloom.glsl"

in vec2 vTexCoord;
out vec4 fragColor;

uniform sampler2D uSceneTexture;

void main()
{
    fragColor = vec4(tonemap(texture(uSceneTexture, vTexCoord).rgb + bloom(vTexCoord)), 1.0);
}
