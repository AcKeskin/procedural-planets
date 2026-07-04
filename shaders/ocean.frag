#version 450 core

in vec3 fragWorldPos;
in vec3 fragNormal;
in float fragDepth01; // real bathymetry: 0 at coast, 1 over the deepest floor

uniform vec3 uCameraPos;
uniform vec3 uLightDir;
uniform vec3 uSunColor;        // sun-of-record color: tints the specular glint
uniform float uSunIntensity;
uniform float uAmbientLight;   // keeps water readable off the key light, matching the land
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

    // Real depth color from the baked bathymetry — shallow (teal) hugging coasts, deep (navy)
    // offshore — instead of the old view-angle fake. sqrt spreads the shelf across more of the
    // visible ocean (a raw exp saturates to deep within a thin coastal ring).
    float depthBlend = sqrt(clamp(fragDepth01, 0.0, 1.0));
    vec3 oceanColor = mix(uShallowColor, uDeepColor, depthBlend);

    float fresnel = pow(1.0 - max(dot(viewDir, waveNormal), 0.0), uFresnelPower);

    // Diffuse + ambient (sphere normal for consistent illumination). The ambient floor is what
    // keeps water reading as lit water rather than a black void away from the key light.
    vec3 lightDir = -normalize(uLightDir); // uLightDir points from the sun; negate → toward it (matches land)
    float diffuse = max(dot(sphereNormal, lightDir), 0.0);
    oceanColor *= uAmbientLight + diffuse * uSunIntensity;

    // Sun-tinted specular glint (wave normal for shimmer). Sun-of-record color, gated by day so it
    // only glints on the lit side.
    vec3 halfVec = normalize(viewDir + lightDir);
    float specularAngle = acos(max(dot(halfVec, waveNormal), 0.0));
    float specularExponent = specularAngle / max(1.0 - uSmoothness, 0.01);
    float specularHighlight = exp(-specularExponent * specularExponent);

    // Grazing angles pick up a cool sky reflection, not just a brighten.
    vec3 skyTint = vec3(0.15, 0.28, 0.42);
    oceanColor = mix(oceanColor, oceanColor + skyTint, fresnel * 0.6);
    oceanColor += uSunColor * (specularHighlight * diffuse) * 1.2;

    // Shoreline foam: a bright surf band in the shallowest water (near the coastline), animated,
    // fading out into open water. Keyed on the real bathymetry so it follows the actual coast.
    float foamBand = 1.0 - smoothstep(0.0, 0.12, fragDepth01);
    float foamAnim = 0.75 + 0.25 * sin(uTime * uWaveSpeed * 2.0 + fragDepth01 * 60.0);
    float foam = foamBand * foamAnim * (uAmbientLight + diffuse * uSunIntensity);
    oceanColor = mix(oceanColor, vec3(0.85, 0.92, 0.95), foam * 0.5);

    // Shallow water reads slightly more transparent than the deep floor.
    float alpha = clamp(0.65 + depthBlend * 0.33, 0.6, 0.98);

    outColor = vec4(oceanColor, alpha);
}
