// Single-scattering sky shared by the atmosphere post pass and the ocean's sky reflection, so both
// read the same parameters the same way. All lengths are world units; the coefficients arrive
// divided by planet radius (AtmosphereRenderer / SetSkyUniforms), so the look is radius-invariant.

uniform vec3 uPlanetCenter;
uniform float uPlanetRadius;
uniform float uAtmosphereRadius;
uniform vec3 uScatteringCoefficients; // Rayleigh, per world unit
uniform float uDensityFalloff;
uniform float uMieCoefficient;        // per world unit
uniform float uMieDensityFalloff;
uniform float uIntensity;

const float SKY_MAX_FLOAT = 3.402823466e+38;

// Returns (distanceToSphere, distanceThroughSphere)
vec2 skyRaySphere(vec3 sphereCenter, float sphereRadius, vec3 rayOrigin, vec3 rayDir)
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
            return vec2(dstToSphereNear, dstToSphereFar - dstToSphereNear);
    }
    return vec2(SKY_MAX_FLOAT, 0.0);
}

// vec2(Rayleigh, Mie) density. The linear (1-h) term forces density to zero at the shell edge.
vec2 skyDensity(vec3 point)
{
    float heightAboveSurface = length(point - uPlanetCenter) - uPlanetRadius;
    float height01 = heightAboveSurface / (uAtmosphereRadius - uPlanetRadius);
    float rayleigh = exp(-height01 * uDensityFalloff) * (1.0 - height01);
    float mie = exp(-height01 * uMieDensityFalloff) * (1.0 - height01);
    return vec2(rayleigh, mie);
}

// vec2(Rayleigh, Mie) optical depth along a ray.
vec2 skyOpticalDepth(vec3 rayOrigin, vec3 rayDir, float rayLength, int samples)
{
    vec3 samplePoint = rayOrigin;
    float stepSize = rayLength / float(samples - 1);
    vec2 depth = vec2(0.0);
    for (int i = 0; i < samples; i++)
    {
        depth += skyDensity(samplePoint) * stepSize;
        samplePoint += rayDir * stepSize;
    }
    return depth;
}

struct SkyScatter
{
    vec3 inScatter;           // light scattered toward the viewer along the ray
    vec2 viewOpticalDepth;    // (Rayleigh, Mie) along the ray, for dimming what lies behind it
};

// In-scatter along a ray segment inside the atmosphere. sunDir points toward the sun.
SkyScatter skyScatter(vec3 rayOrigin, vec3 rayDir, float rayLength, vec3 sunDir, int viewSamples, int sunSamples)
{
    vec3 inScatterPoint = rayOrigin;
    float stepSize = rayLength / float(viewSamples - 1);
    vec3 inScattered = vec3(0.0);
    vec2 viewOpticalDepth = vec2(0.0);

    for (int i = 0; i < viewSamples; i++)
    {
        float sunRayLength = skyRaySphere(uPlanetCenter, uAtmosphereRadius, inScatterPoint, sunDir).y;
        vec2 sunRayOpticalDepth = skyOpticalDepth(inScatterPoint, sunDir, sunRayLength, sunSamples);

        vec2 localDensity = skyDensity(inScatterPoint);
        viewOpticalDepth += localDensity * stepSize;

        vec3 transmittance = exp(-(sunRayOpticalDepth.x + viewOpticalDepth.x) * uScatteringCoefficients
                                - (sunRayOpticalDepth.y + viewOpticalDepth.y) * uMieCoefficient);

        inScattered += localDensity.x * transmittance;
        inScatterPoint += rayDir * stepSize;
    }

    return SkyScatter(inScattered * uScatteringCoefficients * uIntensity * stepSize, viewOpticalDepth);
}
