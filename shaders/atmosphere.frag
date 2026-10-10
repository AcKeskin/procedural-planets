#version 450 core

// lighting.glsl → dayFactor + DUSK_TINT: the sky dusk band reuses the exact curve and warm tint the
// surface shader applies, so the terminator's sky reddening meets the ground twilight band with no split.
#include "lighting.glsl"
#include "tonemap.glsl"
#include "sky_radiance.glsl"

in vec2 vUv;

out vec4 outColor;

uniform sampler2D uSceneTexture;
uniform sampler2D uDepthTexture;
uniform sampler2D uBlueNoise;

uniform float uDitherStrength;
uniform float uDitherScale;

uniform mat4 uInvView;
uniform mat4 uInvProjection;
uniform vec3 uCameraPos;

uniform vec3 uLightDir;          // points FROM the sun, like every other shader
uniform vec3 uSunColor;           // the sun-of-record's color; tints in-scatter and its dusk band

uniform float uOceanRadius;

uniform int uNumInScatteringPoints;
uniform int uNumOpticalDepthPoints;

uniform float uNearPlane;
uniform float uFarPlane;

vec2 squareUV(vec2 uv, vec2 screenSize) {
    const float scale = 1000.0;
    return vec2(uv.x * screenSize.x / scale, uv.y * screenSize.y / scale);
}

// Raymarched in-scattering with brightness-adapted surface compositing
vec3 calculateLight(vec3 rayOrigin, vec3 rayDir, float rayLength, vec3 originalColor, vec2 uv)
{
    vec2 screenSize = vec2(textureSize(uSceneTexture, 0));
    float blueNoise = texture(uBlueNoise, squareUV(uv, screenSize) * uDitherScale).r;
    blueNoise = (blueNoise - 0.5) * uDitherStrength;

    vec3 sunDir = -normalize(uLightDir); // toward the sun
    SkyScatter scatter = skyScatter(rayOrigin, rayDir, rayLength, sunDir, uNumInScatteringPoints, uNumOpticalDepthPoints);
    vec3 inScatteredLight = scatter.inScatter;
    vec2 viewRayOpticalDepth = scatter.viewOpticalDepth;

    // Warm the sky through the twilight band. Rayleigh in-scatter is intrinsically blue, so a
    // multiplicative warm tint would only desaturate it — instead shift its hue toward the warm
    // DUSK_TINT (shared with the surface's duskSunColor), preserving brightness.
    //
    // Keyed on the sun's elevation over the local horizon at the ray's atmosphere entry. Two things
    // keep this a broad, curved glow rather than a hard line: (1) a WIDE twilight half-width — the
    // surface's crisp terminator softness (~0.08) would paint a razor-thin locus that reads as a
    // straight streak edge-on, so the sky band is deliberately much wider; (2) gating by the
    // in-scatter magnitude, so the tint only touches lit atmosphere and never paints the night void.
    const float SKY_DUSK_WIDTH = 0.30;
    vec3 localUp = normalize(rayOrigin - uPlanetCenter);
    float sunElevation = dot(localUp, sunDir);
    float dusk = 1.0 - smoothstep(0.0, SKY_DUSK_WIDTH, abs(sunElevation));
    // Redirect the in-scatter's own brightness into the warm tint (not a fixed color), so the dusk
    // glows where the atmosphere is already lit and stays dark over the night void.
    float inScatterLuma = dot(inScatteredLight, vec3(0.299, 0.587, 0.114));
    vec3 duskTarget = inScatterLuma * DUSK_TINT * 2.2;
    inScatteredLight = mix(inScatteredLight, duskTarget, dusk * 0.75);
    inScatteredLight *= uSunColor;

    inScatteredLight += blueNoise * 0.01;

    // Scalar attenuation avoids per-wavelength color shift (rainbow fringe)
    float avgCoeff = dot(uScatteringCoefficients, vec3(0.333));
    float weightedOpticalDepth = viewRayOpticalDepth.x * avgCoeff;

    // Surface out-scatter: how much the atmosphere dims the surface behind it. Kept gentle so the
    // continents/ocean read through from orbit instead of being washed to a flat teal veil — the
    // long view path across the disc interior otherwise erases the surface entirely.
    const float brightnessAdaptionStrength = 0.15;
    const float reflectedLightOutScatterStrength = 1.0;
    float brightnessAdaption = dot(inScatteredLight, vec3(1.0)) * brightnessAdaptionStrength;
    float brightnessSum = weightedOpticalDepth * uIntensity * reflectedLightOutScatterStrength + brightnessAdaption;
    float reflectedLightStrength = exp(-brightnessSum);

    // Bright surfaces (snow, specular) punch through atmosphere
    float hdrStrength = clamp(dot(originalColor, vec3(1.0)) / 3.0 - 1.0, 0.0, 1.0);
    reflectedLightStrength = mix(reflectedLightStrength, 1.0, hdrStrength);

    return originalColor * reflectedLightStrength + inScatteredLight;
}

vec3 getViewRay(vec2 uv)
{
    vec2 ndc = uv * 2.0 - 1.0;
    vec4 viewSpace = uInvProjection * vec4(ndc, 0.0, 1.0);
    viewSpace.xyz /= viewSpace.w;
    vec3 worldDir = (uInvView * vec4(viewSpace.xyz, 0.0)).xyz;
    return normalize(worldDir);
}

// Reversed-Z: the depth buffer stores near->1, far->0 (see Camera::GetProjectionMatrix).
// Linear eye depth from a reversed [0,1] buffer value is near*far / (near + d*(far-near)) —
// the standard formula with the reversed sample d' = 1-d folded in.
float linearizeDepth(float depth)
{
    return uNearPlane * uFarPlane / (uNearPlane + depth * (uFarPlane - uNearPlane));
}

void main()
{
    vec3 originalColor = texture(uSceneTexture, vUv).rgb;
    float depthSample = texture(uDepthTexture, vUv).r;

    vec3 rayDir = getViewRay(vUv);
    vec3 rayOrigin = uCameraPos;

    float linearDepth = linearizeDepth(depthSample);
    float sceneDepth = linearDepth / dot(rayDir, (uInvView * vec4(0.0, 0.0, -1.0, 0.0)).xyz);

    float dstToOcean = skyRaySphere(uPlanetCenter, uOceanRadius, rayOrigin, rayDir).x;
    float dstToSurface = min(sceneDepth, dstToOcean);

    vec2 atmosphereHit = skyRaySphere(uPlanetCenter, uAtmosphereRadius, rayOrigin, rayDir);
    float dstToAtmosphere = atmosphereHit.x;
    float dstThroughAtmosphere = min(atmosphereHit.y, dstToSurface - dstToAtmosphere);

    if (dstThroughAtmosphere > 0.0)
    {
        const float epsilon = 0.0001;
        vec3 pointInAtmosphere = rayOrigin + rayDir * (dstToAtmosphere + epsilon);
        vec3 light = calculateLight(pointInAtmosphere, rayDir, dstThroughAtmosphere - epsilon * 2.0, originalColor, vUv);
        outColor = vec4(tonemap(light), 1.0);
    }
    else
    {
        outColor = vec4(tonemap(originalColor), 1.0);
    }
}
