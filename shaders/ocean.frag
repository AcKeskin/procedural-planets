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

// Waves run on the direction scaled to this radius, so they keep their size relative to the
// planet at any planet radius (the same rule as the ground's material noise).
const float kWaveReferenceRadius = 10.0;
const int kWaveCount = 8;
const float kBaseWaveFrequency = 300.0; // radians per reference unit at wave scale 1

// Fixed, well-spread 3D wave directions (plane waves through the sphere).
const vec3 kWaveDirs[kWaveCount] = vec3[kWaveCount](
    vec3( 0.83,  0.15,  0.54), vec3(-0.41,  0.79,  0.46), vec3( 0.12, -0.63,  0.77), vec3(-0.92, -0.25,  0.30),
    vec3( 0.35,  0.90, -0.26), vec3(-0.18, -0.21, -0.96), vec3( 0.71, -0.56, -0.42), vec3(-0.55,  0.44, -0.71));

struct WaveSurface
{
    vec3 normal;
    float slopeVariance; // slope of waves too small for a pixel: widens the sun glitter
};

// Sum of plane waves over the sphere. Each wave's height gradient is projected onto the surface
// to tilt the normal. A wave shorter than about two pixels cannot be drawn without shimmer; it
// fades out of the normal and into slopeVariance, so distant water reads as rougher, not flat.
WaveSurface waveSurface(vec3 dir)
{
    vec3 p = dir * kWaveReferenceRadius;
    float footprint = length(fwidth(p)); // reference units per pixel

    vec3 gradient = vec3(0.0);
    float lostVariance = 0.0;
    float frequency = kBaseWaveFrequency * uWaveScale;
    float slope = uWaveStrength;
    for (int k = 0; k < kWaveCount; ++k)
    {
        vec3 d = normalize(kWaveDirs[k]);
        float speed = uWaveSpeed * sqrt(frequency) * 0.5; // longer waves travel faster
        float phase = dot(d, p) * frequency + uTime * speed + float(k) * 1.7;
        float resolved = 1.0 - smoothstep(0.25, 0.5, frequency * footprint / 6.2832);

        gradient += d * (slope * cos(phase) * resolved);
        lostVariance += 0.5 * slope * slope * (1.0 - resolved);

        frequency *= 1.73;
        slope *= 0.7;
    }

    gradient -= dir * dot(gradient, dir); // keep the tilt tangential
    return WaveSurface(normalize(dir - gradient), lostVariance);
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
    WaveSurface waves = waveSurface(sphereNormal);
    vec3 waveNormal = waves.normal;

    // Diffuse + ambient on the sphere normal, matching the land's terminator.
    vec3 lightDir = -normalize(uLightDir); // uLightDir points from the sun; negate → toward it (matches land)
    float diffuse = max(dot(sphereNormal, lightDir), 0.0);
    vec3 color = body.color * (uAmbientLight + diffuse * uSunIntensity);

    // Sun glitter: Blinn-Phong whose width comes from the slopes too small to draw. Up close the
    // lobe is tight and the drawn waves scatter it into glints; from orbit it spreads into the
    // broad glitter patch. Normalised so the patch dims as it widens.
    vec3 halfVec = normalize(viewDir + lightDir);
    float ndh = max(dot(waveNormal, halfVec), 0.0);
    float roughness2 = 0.0005 + waves.slopeVariance;
    float shininess = clamp(2.0 / roughness2 - 2.0, 16.0, 6000.0);
    float glint = pow(ndh, shininess) * (shininess + 8.0) / (8.0 * 3.14159265) * 0.02;

    float F = fresnel(dot(viewDir, waveNormal));
    vec3 skyTint = vec3(0.15, 0.28, 0.42);
    color = mix(color, skyTint * (uAmbientLight + diffuse * uSunIntensity), F);
    color += uSunColor * glint * diffuse * 0.9;

    // Reflection makes grazing water opaque even where it is shallow and clear.
    float alpha = clamp(max(body.opacity, F), 0.0, 1.0);

    outColor = vec4(color, alpha);
}
