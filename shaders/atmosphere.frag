#version 450 core

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

uniform vec3 uLightDir;

uniform vec3 uPlanetCenter;
uniform float uPlanetRadius;
uniform float uAtmosphereRadius;
uniform float uOceanRadius;

uniform vec3 uScatteringCoefficients;
uniform float uDensityFalloff;
uniform float uIntensity;
uniform int uNumInScatteringPoints;
uniform int uNumOpticalDepthPoints;

// Mie contributes to extinction only, not in-scattering color
uniform float uMieCoefficient;
uniform float uMieDensityFalloff;

uniform float uNearPlane;
uniform float uFarPlane;

const float MAX_FLOAT = 3.402823466e+38;

vec2 squareUV(vec2 uv, vec2 screenSize) {
    const float scale = 1000.0;
    return vec2(uv.x * screenSize.x / scale, uv.y * screenSize.y / scale);
}

// Returns (distanceToSphere, distanceThroughSphere)
vec2 raySphere(vec3 sphereCenter, float sphereRadius, vec3 rayOrigin, vec3 rayDir)
{
    vec3 offset = rayOrigin - sphereCenter;
    float b = 2.0 * dot(offset, rayDir);
    float c = dot(offset, offset) - sphereRadius * sphereRadius;
    float discriminant = b * b - 4.0 * c;

    if (discriminant > 0.0)
    {
        float s = sqrt(discriminant);
        float dstToSphereNear = max(0.0, (-b - s) * 0.5);
        float dstToSphereFar = (-b + s) * 0.5;

        if (dstToSphereFar >= 0.0)
        {
            return vec2(dstToSphereNear, dstToSphereFar - dstToSphereNear);
        }
    }
    return vec2(MAX_FLOAT, 0.0);
}

// Returns vec2(Rayleigh, Mie) density
// Linear term (1-h) forces density to zero at atmosphere edge, eliminating fringe artifacts
vec2 densityAtPoint(vec3 point)
{
    float heightAboveSurface = length(point - uPlanetCenter) - uPlanetRadius;
    float height01 = heightAboveSurface / (uAtmosphereRadius - uPlanetRadius);
    float rayleigh = exp(-height01 * uDensityFalloff) * (1.0 - height01);
    float mie = exp(-height01 * uMieDensityFalloff) * (1.0 - height01);
    return vec2(rayleigh, mie);
}

// Returns vec2(Rayleigh, Mie) optical depth along a ray
vec2 opticalDepth(vec3 rayOrigin, vec3 rayDir, float rayLength)
{
    vec3 samplePoint = rayOrigin;
    float stepSize = rayLength / float(uNumOpticalDepthPoints - 1);
    vec2 depth = vec2(0.0);

    for (int i = 0; i < uNumOpticalDepthPoints; i++)
    {
        depth += densityAtPoint(samplePoint) * stepSize;
        samplePoint += rayDir * stepSize;
    }
    return depth;
}

// Raymarched in-scattering with brightness-adapted surface compositing
vec3 calculateLight(vec3 rayOrigin, vec3 rayDir, float rayLength, vec3 originalColor, vec2 uv)
{
    vec2 screenSize = vec2(textureSize(uSceneTexture, 0));
    float blueNoise = texture(uBlueNoise, squareUV(uv, screenSize) * uDitherScale).r;
    blueNoise = (blueNoise - 0.5) * uDitherStrength;

    vec3 inScatterPoint = rayOrigin;
    float stepSize = rayLength / float(uNumInScatteringPoints - 1);
    vec3 inScatteredLight = vec3(0.0);
    vec2 viewRayOpticalDepth = vec2(0.0);

    for (int i = 0; i < uNumInScatteringPoints; i++)
    {
        float sunRayLength = raySphere(uPlanetCenter, uAtmosphereRadius, inScatterPoint, uLightDir).y;
        vec2 sunRayOpticalDepth = opticalDepth(inScatterPoint, uLightDir, sunRayLength);

        vec2 localDensity = densityAtPoint(inScatterPoint);
        viewRayOpticalDepth += localDensity * stepSize;

        // Combined Rayleigh + Mie extinction
        vec3 transmittance = exp(-(sunRayOpticalDepth.x + viewRayOpticalDepth.x) * uScatteringCoefficients
                                - (sunRayOpticalDepth.y + viewRayOpticalDepth.y) * uMieCoefficient);

        inScatteredLight += localDensity.x * transmittance;
        inScatterPoint += rayDir * stepSize;
    }

    inScatteredLight *= uScatteringCoefficients * uIntensity * stepSize;
    inScatteredLight += blueNoise * 0.01;

    // Scalar attenuation avoids per-wavelength color shift (rainbow fringe)
    float avgCoeff = dot(uScatteringCoefficients, vec3(0.333));
    float weightedOpticalDepth = viewRayOpticalDepth.x * avgCoeff;

    const float brightnessAdaptionStrength = 0.15;
    const float reflectedLightOutScatterStrength = 3.0;
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

float linearizeDepth(float depth)
{
    return uNearPlane * uFarPlane / (uFarPlane - depth * (uFarPlane - uNearPlane));
}

void main()
{
    vec3 originalColor = texture(uSceneTexture, vUv).rgb;
    float depthSample = texture(uDepthTexture, vUv).r;

    vec3 rayDir = getViewRay(vUv);
    vec3 rayOrigin = uCameraPos;

    float linearDepth = linearizeDepth(depthSample);
    float sceneDepth = linearDepth / dot(rayDir, (uInvView * vec4(0.0, 0.0, -1.0, 0.0)).xyz);

    float dstToOcean = raySphere(uPlanetCenter, uOceanRadius, rayOrigin, rayDir).x;
    float dstToSurface = min(sceneDepth, dstToOcean);

    vec2 atmosphereHit = raySphere(uPlanetCenter, uAtmosphereRadius, rayOrigin, rayDir);
    float dstToAtmosphere = atmosphereHit.x;
    float dstThroughAtmosphere = min(atmosphereHit.y, dstToSurface - dstToAtmosphere);

    if (dstThroughAtmosphere > 0.0)
    {
        const float epsilon = 0.0001;
        vec3 pointInAtmosphere = rayOrigin + rayDir * (dstToAtmosphere + epsilon);
        vec3 light = calculateLight(pointInAtmosphere, rayDir, dstThroughAtmosphere - epsilon * 2.0, originalColor, vUv);
        outColor = vec4(light, 1.0);
    }
    else
    {
        outColor = vec4(originalColor, 1.0);
    }
}
