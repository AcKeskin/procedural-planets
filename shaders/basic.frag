#version 450 core

in vec3 vNormal;
in vec3 vWorldPos;

out vec4 FragColor;

uniform vec3 uLightDir = vec3(0.5, 1.0, 0.3);
uniform vec3 uBaseColor = vec3(0.4, 0.6, 0.8);

void main()
{
    vec3 normal = normalize(vNormal);
    vec3 lightDir = -normalize(uLightDir);

    float diff = max(dot(normal, lightDir), 0.0);
    float ambient = 0.2;

    vec3 color = uBaseColor * (ambient + diff * 0.8);
    FragColor = vec4(color, 1.0);
}
