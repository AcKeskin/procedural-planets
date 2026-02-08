#version 450 core

in vec3 fragWorldPos;
in vec3 fragNormal;

uniform vec3 uCameraPos;
uniform vec3 uLightDir;
uniform vec3 uDeepColor;
uniform vec3 uShallowColor;
uniform float uFresnelPower;
uniform float uDepthMultiplier;
uniform float uAlphaMultiplier;
uniform float uSmoothness;
uniform float uWaveStrength;
uniform float uWaveScale;
uniform float uWaveSpeed;
uniform float uTime;

out vec4 outColor;

// Two sine layers at different frequencies for procedural wave normals
vec3 calcWaveNormal(vec3 worldPos, vec3 sphereNormal)
{
    vec3 up = abs(sphereNormal.y) < 0.999 ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangent1 = normalize(cross(sphereNormal, up));
    vec3 tangent2 = cross(sphereNormal, tangent1);

    float u = dot(worldPos, tangent1);
    float v = dot(worldPos, tangent2);

    // Large slow waves
    float wave1u = sin(u * uWaveScale * 0.1 + uTime * uWaveSpeed)
                 * cos(v * uWaveScale * 0.13 + uTime * uWaveSpeed * 0.8);
    float wave1v = cos(u * uWaveScale * 0.11 - uTime * uWaveSpeed * 0.9)
                 * sin(v * uWaveScale * 0.1 + uTime * uWaveSpeed * 0.7);

    // Smaller faster waves
    float wave2u = sin(u * uWaveScale * 0.3 - uTime * uWaveSpeed * 1.3)
                 * cos(v * uWaveScale * 0.27 - uTime * uWaveSpeed * 0.6);
    float wave2v = cos(u * uWaveScale * 0.28 + uTime * uWaveSpeed * 1.1)
                 * sin(v * uWaveScale * 0.32 - uTime * uWaveSpeed * 0.9);

    vec3 perturbation = tangent1 * (wave1u * 0.6 + wave2u * 0.4)
                      + tangent2 * (wave1v * 0.6 + wave2v * 0.4);

    return normalize(mix(sphereNormal, sphereNormal + perturbation, uWaveStrength));
}

void main()
{
    vec3 viewDir = normalize(uCameraPos - fragWorldPos);
    vec3 sphereNormal = normalize(fragNormal);
    vec3 waveNormal = calcWaveNormal(fragWorldPos, sphereNormal);

    // Approximate depth from viewing angle (perpendicular = more water)
    float viewDot = max(dot(viewDir, sphereNormal), 0.0);
    float approxDepth = (1.0 - viewDot) * uDepthMultiplier;

    // Exponential depth color blend
    vec3 oceanColor = mix(uShallowColor, uDeepColor, 1.0 - exp(-approxDepth));

    float fresnel = pow(1.0 - max(dot(viewDir, waveNormal), 0.0), uFresnelPower);

    // Diffuse (sphere normal for consistent illumination)
    vec3 lightDir = -normalize(uLightDir);
    float diffuse = max(dot(sphereNormal, lightDir), 0.0);
    oceanColor *= diffuse;

    // Smoothness-based specular (wave normal for shimmer)
    vec3 halfVec = normalize(viewDir + lightDir);
    float specularAngle = acos(max(dot(halfVec, waveNormal), 0.0));
    float specularExponent = specularAngle / max(1.0 - uSmoothness, 0.01);
    float specularHighlight = exp(-specularExponent * specularExponent);

    oceanColor = mix(oceanColor, oceanColor + vec3(0.05, 0.1, 0.15), fresnel * 0.5);
    oceanColor += vec3(specularHighlight) * 0.9;

    // Shallow water more transparent
    float alpha = 1.0 - exp(-approxDepth * uAlphaMultiplier / max(uDepthMultiplier, 0.01));
    alpha = clamp(alpha, 0.6, 0.98);

    outColor = vec4(oceanColor, alpha);
}
