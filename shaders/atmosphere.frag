#version 450 core

in vec3 fragWorldPos;
in vec3 fragNormal;

uniform vec3 uCameraPos;
uniform vec3 uLightDir;
uniform vec3 uAtmosphereColor;
uniform float uDensityFalloff;
uniform float uIntensity;

out vec4 outColor;

void main()
{
    vec3 viewDir = normalize(uCameraPos - fragWorldPos);
    vec3 normal = normalize(fragNormal);

    // Rim intensity - brighter at edges
    float viewAngle = 1.0 - max(dot(viewDir, normal), 0.0);
    float rim = pow(viewAngle, uDensityFalloff);

    // Day/night mask - atmosphere is lit on sun side
    float sunFacing = max(dot(normal, uLightDir), 0.0);
    float daylight = smoothstep(-0.1, 0.3, sunFacing);

    // Scatter color (slight blue shift)
    vec3 scatterColor = uAtmosphereColor;

    // Final alpha
    float alpha = rim * daylight * uIntensity;

    // Clamp alpha to avoid over-saturation
    alpha = min(alpha, 0.9);

    outColor = vec4(scatterColor, alpha);
}
