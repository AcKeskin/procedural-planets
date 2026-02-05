#version 450 core

// Passthrough fragment shader for direct texture blit
// No post-processing applied - serves as base for future effects

in vec2 vTexCoord;
out vec4 fragColor;

uniform sampler2D uSceneTexture;

void main()
{
    // Sample scene texture directly without modification
    fragColor = texture(uSceneTexture, vTexCoord);
}
