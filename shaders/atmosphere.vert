#version 450 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUv;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;

out vec3 fragWorldPos;
out vec3 fragNormal;

void main()
{
    vec4 worldPos = uModel * vec4(aPosition, 1.0);
    fragWorldPos = worldPos.xyz;
    fragNormal = normalize(worldPos.xyz);  // Sphere normal = position direction

    gl_Position = uProjection * uView * worldPos;
}
