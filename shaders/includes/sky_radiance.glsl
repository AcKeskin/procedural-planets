// Single-scattering sky shared by the atmosphere post pass and the ocean's sky reflection, so both
// read the same parameters the same way. All lengths are world units; the coefficients arrive
// divided by planet radius (AtmosphereRenderer / SetSkyUniforms), so the look is radius-invariant.

uniform vec3 uPlanetCenter;
uniform float uPlanetRadius;
uniform float uGroundRadius;          // the sphere that casts the earth shadow: sea level, or the planet radius
uniform float uAtmosphereRadius;
uniform vec3 uScatteringCoefficients; // Rayleigh, per world unit
uniform float uDensityFalloff;
uniform float uMieCoefficient;        // per world unit
uniform float uMieDensityFalloff;
uniform vec3 uMieAlbedo;              // colour the haze scatters (white on Earth)
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

// Phase functions, scaled so their average over the sphere is 1 (uIntensity keeps its meaning).
const float SKY_MIE_G = 0.76; // forward lobe of haze: the bright halo around the sun
// Light scattered more than once, as a share of single scattering. Lifts the sky overhead and
// fills the earth shadow at twilight without a second march.
const float SKY_MULTI_SCATTER = 0.3;

float skyRayleighPhase(float cosTheta)
{
    return 0.75 * (1.0 + cosTheta * cosTheta);
}

float skyMiePhase(float cosTheta)
{
    float g2 = SKY_MIE_G * SKY_MIE_G;
    return (1.0 - g2) / pow(1.0 + g2 - 2.0 * SKY_MIE_G * cosTheta, 1.5);
}

// Per-channel extinction for a (Rayleigh, Mie) optical depth.
vec3 skyTransmittance(vec2 opticalDepth)
{
    return exp(-opticalDepth.x * uScatteringCoefficients - opticalDepth.y * uMieCoefficient);
}

// 1 where the sun clears the planet as seen from p, 0 where the planet blocks it. Compares the
// sun's height with the horizon dip at p's altitude; soft across the horizon, so samples at or
// below the surface fade with the terminator instead of switching at its plane.
float skySunVisibility(vec3 p, vec3 sunDir)
{
    vec3 offset = p - uPlanetCenter;
    float r = length(offset);
    float sunHeight = dot(offset / r, sunDir);
    float ratio = min(uGroundRadius / r, 1.0);
    float horizon = -sqrt(1.0 - ratio * ratio);
    return smoothstep(horizon - 0.04, horizon + 0.04, sunHeight);
}

struct SkyScatter
{
    vec3 inScatter;           // light scattered toward the viewer along the ray
    vec3 transmittance;       // per-channel extinction along the ray, for what lies behind it
};

// Single scattering along a ray segment inside the atmosphere. sunDir points toward the sun. Each
// sample reads its own sun path, so low sun reddens the sky near the sun and nowhere else.
SkyScatter skyScatter(vec3 rayOrigin, vec3 rayDir, float rayLength, vec3 sunDir, int viewSamples, int sunSamples)
{
    vec3 inScatterPoint = rayOrigin;
    float stepSize = rayLength / float(viewSamples - 1);
    vec3 rayleigh = vec3(0.0);
    vec3 mie = vec3(0.0);
    vec3 ambient = vec3(0.0);
    vec2 viewOpticalDepth = vec2(0.0);

    for (int i = 0; i < viewSamples; i++)
    {
        vec2 localDensity = skyDensity(inScatterPoint);
        viewOpticalDepth += localDensity * stepSize;

        // Multiple scattering: isotropic, from the whole lit sky above the sample, so it follows
        // the local sun height and fades a little past the terminator instead of at it.
        float sunHeight = dot(normalize(inScatterPoint - uPlanetCenter), sunDir);
        vec3 localScatter = localDensity.x * uScatteringCoefficients + localDensity.y * uMieCoefficient * uMieAlbedo;
        ambient += localScatter * skyTransmittance(viewOpticalDepth) * smoothstep(-0.2, 0.3, sunHeight);

        float visibility = skySunVisibility(inScatterPoint, sunDir);
        if (visibility > 0.0)
        {
            float sunRayLength = skyRaySphere(uPlanetCenter, uAtmosphereRadius, inScatterPoint, sunDir).y;
            vec2 sunRayOpticalDepth = skyOpticalDepth(inScatterPoint, sunDir, sunRayLength, sunSamples);
            vec3 transmittance = skyTransmittance(sunRayOpticalDepth + viewOpticalDepth) * visibility;
            rayleigh += localDensity.x * transmittance;
            mie += localDensity.y * transmittance;
        }
        inScatterPoint += rayDir * stepSize;
    }

    float cosTheta = dot(rayDir, sunDir);
    vec3 inScatter = rayleigh * uScatteringCoefficients * skyRayleighPhase(cosTheta)
                   + mie * uMieCoefficient * uMieAlbedo * skyMiePhase(cosTheta)
                   + ambient * SKY_MULTI_SCATTER;
    return SkyScatter(inScatter * uIntensity * stepSize, skyTransmittance(viewOpticalDepth));
}
