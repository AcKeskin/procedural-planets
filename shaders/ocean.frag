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

// The reflected sky is the atmosphere pass's own sky (sky_radiance.glsl); 0 = airless, black sky.
uniform int uAtmosphereOn;

#include "sky_radiance.glsl"

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
const int kWaveCount = 16;
const float kBaseWaveFrequency = 300.0; // radians per reference unit at wave scale 1

// Random, well-spread 3D directions with jittered frequencies and phases: irregular ratios keep
// the waves from interfering into a regular grid.
const vec3 kWaveDirs[kWaveCount] = vec3[kWaveCount](
    vec3( 0.55, -0.35,  0.76), vec3( 0.86,  0.30,  0.42), vec3(-0.66,  0.07,  0.75), vec3(-0.47, -0.88, -0.02),
    vec3(-0.35, -0.93,  0.15), vec3( 0.43, -0.86,  0.28), vec3( 0.46, -0.15, -0.88), vec3( 0.11, -0.75,  0.65),
    vec3( 0.92,  0.25, -0.31), vec3(-0.79,  0.15,  0.60), vec3( 0.29,  0.95,  0.09), vec3(-0.17,  0.72,  0.68),
    vec3( 0.52, -0.71,  0.47), vec3( 0.37, -0.38, -0.85), vec3(-0.67, -0.64, -0.38), vec3(-0.67,  0.28,  0.69));
const float kWaveJitter[kWaveCount] = float[kWaveCount](1.01, 0.87, 0.87, 0.91, 1.05, 0.98, 0.94, 1.03, 0.99, 0.94, 1.09, 1.06, 0.92, 1.02, 1.01, 1.11);
const float kWavePhase[kWaveCount] = float[kWaveCount](4.58, 1.81, 6.16, 0.74, 2.63, 4.75, 0.95, 3.07, 0.25, 4.20, 4.80, 3.60, 5.50, 1.97, 4.37, 3.73);

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
    // Larger pixel axis: at grazing angles one axis stretches far, and the average would let
    // waves through that the pixel cannot hold (moire).
    float footprint = max(length(dFdx(p)), length(dFdy(p))); // reference units per pixel

    // Low-frequency domain warp: bends the crests so no wave stays a straight line.
    float f0 = kBaseWaveFrequency * uWaveScale;
    vec3 warp = vec3(sin(dot(p, vec3(0.8, 0.3, 0.5)) * f0 * 0.11),
                     sin(dot(p, vec3(-0.4, 0.9, 0.2)) * f0 * 0.13),
                     sin(dot(p, vec3(0.3, -0.2, 0.9)) * f0 * 0.09)) * (2.0 / f0);
    vec3 pw = p + warp;

    vec3 gradient = vec3(0.0);
    float lostVariance = 0.0;
    float frequency = f0;
    float slope = uWaveStrength * 0.6;
    for (int k = 0; k < kWaveCount; ++k)
    {
        vec3 d = normalize(kWaveDirs[k]);
        float f = frequency * kWaveJitter[k];
        float speed = uWaveSpeed * sqrt(f) * 0.5; // longer waves travel faster
        float phase = dot(d, pw) * f + uTime * speed + kWavePhase[k];
        float resolved = 1.0 - smoothstep(0.25, 0.5, f * footprint / 6.2832);

        gradient += d * (slope * cos(phase) * resolved);
        lostVariance += 0.5 * slope * slope * (1.0 - resolved);

        frequency *= 1.32;
        slope *= 0.84;
    }

    gradient -= dir * dot(gradient, dir); // keep the tilt tangential
    return WaveSurface(normalize(dir - gradient), lostVariance);
}

// ---- shoreline ----

const float kFoamBandUnits = 0.02; // terrain units of depth the surf band covers

// Surf on the shallowest water, broken up by the wave pattern so it is not a clean line.
// Opacity already falls to 0 at depth 0 (waterBody), so the coast never shows a hard cut.
float shoreFoam(float depthUnits, vec3 dir)
{
    float band = 1.0 - smoothstep(0.0, kFoamBandUnits, depthUnits);
    vec3 p = dir * kWaveReferenceRadius * kBaseWaveFrequency * 0.05;
    float breakup = 0.5 + 0.5 * sin(p.x + p.y * 1.3 + p.z * 0.7 + uTime * uWaveSpeed * 2.0);
    return band * mix(0.4, 1.0, breakup);
}

// ---- reflection ----

// Fewer samples than the atmosphere pass: the reflection is blurred by waves and fresnel anyway.
const int kSkyViewSamples = 4;
const int kSkySunSamples = 3;

// Sky seen along rayDir from the sea surface: the same march as the atmosphere pass, so the
// reflection matches the sky above it at any sun height.
vec3 skyRadiance(vec3 origin, vec3 rayDir, vec3 sunDir)
{
    if (uAtmosphereOn == 0)
        return vec3(0.0);
    vec2 shell = skyRaySphere(uPlanetCenter, uAtmosphereRadius, origin, rayDir);
    if (shell.y <= 0.0)
        return vec3(0.0);
    // A reflected ray that dips below the horizon ends on the sea sphere it starts from.
    float seaRadius = length(origin - uPlanetCenter) * 0.9999;
    float toPlanet = skyRaySphere(uPlanetCenter, seaRadius, origin, rayDir).x;
    float segment = min(shell.y, toPlanet - shell.x);
    if (segment <= 0.0)
        return vec3(0.0);
    vec3 start = origin + rayDir * shell.x;
    return skyScatter(start, rayDir, segment, sunDir, kSkyViewSamples, kSkySunSamples).inScatter * uSunColor;
}

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
    float glint = pow(ndh, shininess) * (shininess + 8.0) / (8.0 * 3.14159265) * 0.25;

    float F = fresnel(dot(viewDir, waveNormal));
    vec3 sky = skyRadiance(fragWorldPos, reflect(-viewDir, waveNormal), lightDir);
    color = mix(color, sky, F);
    color += uSunColor * glint * diffuse * 0.9;

    float foam = shoreFoam(depthUnits, sphereNormal);
    color = mix(color, vec3(0.85, 0.92, 0.95) * (uAmbientLight + diffuse * uSunIntensity), foam * 0.6);

    // Reflection makes grazing water opaque even where it is shallow and clear; foam is opaque.
    float alpha = clamp(max(max(body.opacity, F), foam * 0.6), 0.0, 1.0);

    outColor = vec4(color, alpha);
}
