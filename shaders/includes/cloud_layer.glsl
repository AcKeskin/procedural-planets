// The cloud layer's density field: a weather cube map (coverage + type on the sphere) times a
// height profile, carved by tiling 3D shape and detail noise (CloudVolume bakes all three).
// Shared by the cloud march and the ground shadow so both see the same clouds. Lengths are in
// world units; the noise runs on positions divided by the planet radius, so the look is the same
// at any radius.

#ifndef CLOUD_LAYER_GLSL
#define CLOUD_LAYER_GLSL

layout(binding = 8) uniform samplerCube uCloudWeather; // r: coverage, g: type (0 stratus, 1 cumulus)
layout(binding = 9) uniform sampler3D uCloudShape;     // Perlin-Worley
layout(binding = 10) uniform sampler3D uCloudDetail;   // Worley fBm

uniform vec3  uCloudCenter;
uniform float uCloudPlanetRadius;
uniform float uCloudBaseRadius;  // world units
uniform float uCloudTopRadius;   // world units
uniform float uCloudCoverage;    // 0 clear .. 1 overcast
uniform float uCloudDensity;     // extinction scale
uniform float uCloudShapeScale;  // shape tiles per planet radius
uniform float uCloudDrift;       // radians the weather has turned (time * speed)

const float CLOUD_DETAIL_SCALE = 5.3; // detail tiles per shape tile
const float CLOUD_LARGE_SCALE = 0.19; // the large shape octave, relative to the shape tile

// The weather turns slowly about the planet's axis; the 3D noise moves with it.
vec3 cloudDrift(vec3 v)
{
    float c = cos(uCloudDrift), s = sin(uCloudDrift);
    return vec3(c * v.x - s * v.z, v.y, s * v.x + c * v.z);
}

// Coverage after the global setting, in [0, 1]. The weather coverage is equalized, so a setting
// of 0.45 makes about 45% of the sphere cloudy before the noise carves it.
float cloudCover(float weatherCoverage)
{
    return smoothstep(1.0 - uCloudCoverage - 0.1, 1.0 - uCloudCoverage + 0.3, weatherCoverage);
}

// Height profile: stratus is a thin low deck, cumulus fills most of the slab with a rounded top.
float cloudProfile(float h, float type)
{
    float bottom = smoothstep(0.0, 0.07, h);
    float stratus = 1.0 - smoothstep(0.2, 0.35, h);
    float cumulus = 1.0 - smoothstep(0.55, 1.0, h);
    return bottom * mix(stratus, cumulus, type);
}

float cloudRemap(float v, float lo, float hi)
{
    return clamp((v - lo) / max(hi - lo, 1e-4), 0.0, 1.0);
}

// Density at a world position (0 outside the slab). footprint = world size of one pixel at p:
// noise finer than that is read from coarser mips and the detail fades out, so distant clouds
// don't sparkle. detail = false skips the detail erosion, for the cheap light march.
float cloudDensity(vec3 p, float footprint, bool detail)
{
    vec3 offset = p - uCloudCenter;
    float r = length(offset);
    float h = (r - uCloudBaseRadius) / (uCloudTopRadius - uCloudBaseRadius);
    if (h <= 0.0 || h >= 1.0)
        return 0.0;

    vec3 dir = cloudDrift(offset / r);
    vec2 weather = texture(uCloudWeather, dir).rg;
    float cover = cloudCover(weather.r) * cloudProfile(h, weather.g);
    if (cover <= 0.0)
        return 0.0;

    vec3 q = cloudDrift(offset) / uCloudPlanetRadius * uCloudShapeScale;
    // Texels per pixel at the shape and detail resolutions (64 and 32 per tile).
    float shapeTexels = footprint / uCloudPlanetRadius * uCloudShapeScale * 64.0;
    // Two shape octaves: individual clouds, and a 5x larger one that still breaks the weather
    // into separate clouds from mid orbit. Each fades to a constant once it is too small to read,
    // or the tile repeat would print as a regular stipple.
    float small = textureLod(uCloudShape, q, log2(max(shapeTexels, 1.0))).r;
    small = mix(small, 0.62, smoothstep(1.0, 3.0, shapeTexels));
    float largeTexels = shapeTexels * CLOUD_LARGE_SCALE;
    float large = textureLod(uCloudShape, q * CLOUD_LARGE_SCALE + vec3(0.37, 0.11, 0.73), log2(max(largeTexels, 1.0))).r;
    large = mix(large, 0.62, smoothstep(1.0, 3.0, largeTexels));
    float shape = 0.5 * small + 0.5 * large;
    float d = cloudRemap(shape, 1.0 - cover, 1.0) * cover;
    float detailTexels = shapeTexels * CLOUD_DETAIL_SCALE * 0.5;
    float detailWeight = 1.0 - smoothstep(1.0, 4.0, detailTexels);
    if (detail && d > 0.0 && detailWeight > 0.0)
    {
        // Wispy at the base, billowy at the top.
        float n = textureLod(uCloudDetail, q * CLOUD_DETAIL_SCALE, log2(max(detailTexels, 1.0))).r;
        float erode = mix(1.0 - n, n, clamp(h * 4.0, 0.0, 1.0)) * 0.35 * detailWeight;
        d = cloudRemap(d, erode, 1.0);
    }
    return d * uCloudDensity;
}

// Rough column coverage along a direction, for the ground shadow: weather and profile only.
float cloudColumn(vec3 dir)
{
    vec2 weather = texture(uCloudWeather, cloudDrift(dir)).rg;
    return cloudCover(weather.r) * mix(0.5, 1.0, weather.g);
}

#endif
