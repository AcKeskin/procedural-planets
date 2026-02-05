#version 450 core

in vec3 fragWorldPos;
in vec3 fragNormal;

uniform vec3 uCameraPos;
uniform vec3 uLightDir;
uniform vec3 uDeepColor;
uniform vec3 uShallowColor;
uniform float uFresnelPower;
uniform float uSpecularPower;

out vec4 outColor;

void main()
{
    vec3 viewDir = normalize(uCameraPos - fragWorldPos);
    vec3 normal = normalize(fragNormal);

    // Fresnel effect - more reflection at glancing angles
    float fresnel = pow(1.0 - max(dot(viewDir, normal), 0.0), uFresnelPower);

    // Specular highlight (sun reflection)
    vec3 lightDir = -normalize(uLightDir);
    vec3 halfVec = normalize(viewDir + lightDir);
    float spec = pow(max(dot(normal, halfVec), 0.0), uSpecularPower);

    // Basic diffuse lighting
    float diffuse = max(dot(normal, lightDir), 0.0) * 0.5 + 0.5;

    // Color blend based on fresnel
    vec3 oceanColor = mix(uDeepColor, uShallowColor, fresnel * 0.5);
    oceanColor *= diffuse;

    // Add specular highlight
    vec3 finalColor = oceanColor + vec3(spec) * 0.8;

    // Slight transparency for depth
    float alpha = 0.92 + fresnel * 0.08;

    outColor = vec4(finalColor, alpha);
}
