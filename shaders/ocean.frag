#version 450 core

// Ocean surface, drawn over the terrain patches (ocean.vert). Three parts:
//   water body  - colour and opacity from the real water depth (Beer-Lambert absorption)
//   waves       - surface normal for lighting
//   reflection  - fresnel-weighted sky + sun glitter

in vec3 fragWorldPos;
in vec3 fragNormal;  // unit sphere direction (interpolated)
in float fragDepth;  // water depth in planet-radius units, from the drawn terrain heights

uniform float uHeightScale; // body shape.heightScale: radius units per terrain unit

uniform vec3 uCameraPos;
uniform vec3 uLightDir;
uniform vec3 uSunColor;      // sun-of-record color: tints the specular glint
uniform float uSunIntensity;
uniform float uAmbientLight; // keeps water readable off the key light, matching the land
uniform vec3 uDeepColor;
uniform vec3 uShallowColor;
uniform float uClarity;      // terrain units of depth light travels before it is mostly absorbed
uniform float uWaveStrength;
uniform float uWaveScale;
uniform float uWaveSpeed;
uniform float uTime;

out vec4 outColor;

// ---- water body ----

struct WaterBody
{
    vec3 color;
    float opacity; // 0 = see the seabed, 1 = opaque
};

// depthUnits: water depth in terrain units. Light that reaches the seabed and comes back is
// exp(-2 * depth / clarity); the rest is the water's own colour, shallow teal shading to deep navy.
WaterBody waterBody(float depthUnits)
{
    float clarity = max(uClarity, 1e-4);
    float transmittance = exp(-2.0 * depthUnits / clarity);
    float deepness = 1.0 - exp(-depthUnits / (clarity * 3.0));
    return WaterBody(mix(uShallowColor, uDeepColor, deepness), 1.0 - transmittance);
}

// ---- waves ----

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

// ---- reflection ----

// Schlick fresnel for water (F0 = 0.02).
float fresnel(float cosTheta)
{
    return 0.02 + 0.98 * pow(1.0 - clamp(cosTheta, 0.0, 1.0), 5.0);
}

void main()
{
    if (fragDepth <= 0.0)
        discard; // land: the terrain draws here

    float depthUnits = fragDepth / uHeightScale;
    WaterBody body = waterBody(depthUnits);

    vec3 viewDir = normalize(uCameraPos - fragWorldPos);
    vec3 sphereNormal = normalize(fragNormal);
    vec3 waveNormal = calcWaveNormal(fragWorldPos, sphereNormal);

    // Diffuse + ambient on the sphere normal, matching the land's terminator.
    vec3 lightDir = -normalize(uLightDir); // uLightDir points from the sun; negate → toward it (matches land)
    float diffuse = max(dot(sphereNormal, lightDir), 0.0);
    vec3 color = body.color * (uAmbientLight + diffuse * uSunIntensity);

    // Sun glitter: a broad soft patch plus a tight sparkle over the wave normal.
    vec3 halfVec = normalize(viewDir + lightDir);
    float ndh = max(dot(waveNormal, halfVec), 0.0);
    float glint = pow(ndh, 60.0) * 0.5 + pow(ndh, 900.0) * 0.5;

    float F = fresnel(dot(viewDir, waveNormal));
    vec3 skyTint = vec3(0.15, 0.28, 0.42);
    color = mix(color, skyTint * (uAmbientLight + diffuse * uSunIntensity), F);
    color += uSunColor * glint * diffuse * 0.9;

    // Reflection makes grazing water opaque even where it is shallow and clear.
    float alpha = clamp(max(body.opacity, F), 0.0, 1.0);

    outColor = vec4(color, alpha);
}
