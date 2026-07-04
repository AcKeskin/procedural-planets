#version 450 core

layout(location = 0) in vec3 aPosition; // unit-sphere position
layout(location = 1) in vec3 aNormal;

uniform mat4 uView;
uniform mat4 uProjection;
uniform float uShellRadius;

out vec3 fragWorldPos;
out vec3 fragDir; // unit direction from planet center (coverage-field lookup)

void main()
{
    fragDir = normalize(aPosition);
    vec3 worldPos = fragDir * uShellRadius;
    fragWorldPos = worldPos;
    gl_Position = uProjection * uView * vec4(worldPos, 1.0);
}
