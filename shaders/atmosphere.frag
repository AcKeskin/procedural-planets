#version 450 core

in vec2 vUv;

out vec4 outColor;

// Scene textures
uniform sampler2D uSceneTexture;
uniform sampler2D uDepthTexture;
uniform sampler2D uBlueNoise;

// Dithering parameters
uniform float uDitherStrength;  // Default: 0.8
uniform float uDitherScale;     // Default: 4.0

// Camera
uniform mat4 uInvView;
uniform mat4 uInvProjection;
uniform vec3 uCameraPos;

// Light
uniform vec3 uLightDir;

// Planet geometry
uniform vec3 uPlanetCenter;
uniform float uPlanetRadius;
uniform float uAtmosphereRadius;
uniform float uOceanRadius;

// Scattering parameters
uniform vec3 uScatteringCoefficients;
uniform float uDensityFalloff;
uniform float uIntensity;
uniform int uNumInScatteringPoints;
uniform int uNumOpticalDepthPoints;

// Camera planes for depth reconstruction
uniform float uNearPlane;
uniform float uFarPlane;

const float MAX_FLOAT = 3.402823466e+38;

// Convert UV to square-scaled coordinates for blue noise tiling
vec2 squareUV(vec2 uv, vec2 screenSize) {
    const float scale = 1000.0;
    return vec2(uv.x * screenSize.x / scale, uv.y * screenSize.y / scale);
}

// Ray-sphere intersection
// Returns (distanceToSphere, distanceThroughSphere)
// If ray origin is inside sphere, distanceToSphere = 0
// If ray misses sphere, distanceToSphere = MAX_FLOAT, distanceThroughSphere = 0
vec2 raySphere(vec3 sphereCenter, float sphereRadius, vec3 rayOrigin, vec3 rayDir)
{
    vec3 offset = rayOrigin - sphereCenter;
    float a = 1.0; // dot(rayDir, rayDir) = 1 if normalized
    float b = 2.0 * dot(offset, rayDir);
    float c = dot(offset, offset) - sphereRadius * sphereRadius;
    float discriminant = b * b - 4.0 * a * c;

    if (discriminant > 0.0)
    {
        float s = sqrt(discriminant);
        float dstToSphereNear = max(0.0, (-b - s) / (2.0 * a));
        float dstToSphereFar = (-b + s) / (2.0 * a);

        if (dstToSphereFar >= 0.0)
        {
            return vec2(dstToSphereNear, dstToSphereFar - dstToSphereNear);
        }
    }
    return vec2(MAX_FLOAT, 0.0);
}

// Atmospheric density at a given point
// Pure exponential falloff matching real atmospheric scale height model
float densityAtPoint(vec3 point)
{
    float heightAboveSurface = length(point - uPlanetCenter) - uPlanetRadius;
    float scaleHeight = (uAtmosphereRadius - uPlanetRadius) / uDensityFalloff;
    return exp(-heightAboveSurface / scaleHeight);
}

// Calculate optical depth along a ray
// This represents how much light is absorbed/scattered along the path
float opticalDepth(vec3 rayOrigin, vec3 rayDir, float rayLength)
{
    vec3 samplePoint = rayOrigin;
    float stepSize = rayLength / float(uNumOpticalDepthPoints - 1);
    float depth = 0.0;

    for (int i = 0; i < uNumOpticalDepthPoints; i++)
    {
        float localDensity = densityAtPoint(samplePoint);
        depth += localDensity * stepSize;
        samplePoint += rayDir * stepSize;
    }
    return depth;
}

// Rayleigh phase function: P(theta) = 3/4 * (1 + cos^2(theta))
// Normalized so average over sphere equals 1
float rayleighPhase(float cosTheta)
{
    return 0.75 * (1.0 + cosTheta * cosTheta);
}

// Calculate in-scattered light along the view ray
// Compositing follows physical transmittance: result = scene * T + scattered
vec3 calculateLight(vec3 rayOrigin, vec3 rayDir, float rayLength, vec3 originalColor, vec2 uv)
{
    // Blue noise dithering
    vec2 screenSize = vec2(textureSize(uSceneTexture, 0));
    float blueNoise = texture(uBlueNoise, squareUV(uv, screenSize) * uDitherScale).r;
    blueNoise = (blueNoise - 0.5) * uDitherStrength;

    // Rayleigh phase — brighter toward and away from the sun
    float cosTheta = dot(rayDir, uLightDir);
    float phase = rayleighPhase(cosTheta);

    vec3 inScatterPoint = rayOrigin;
    float stepSize = rayLength / float(uNumInScatteringPoints - 1);
    vec3 inScatteredLight = vec3(0.0);
    float viewRayOpticalDepth = 0.0;

    for (int i = 0; i < uNumInScatteringPoints; i++)
    {
        // Distance from this point to the sun (through atmosphere)
        float sunRayLength = raySphere(uPlanetCenter, uAtmosphereRadius, inScatterPoint, uLightDir).y;

        // Optical depth from this point to the sun
        float sunRayOpticalDepth = opticalDepth(inScatterPoint, uLightDir, sunRayLength);

        // Optical depth from camera to this point (accumulated incrementally)
        float localDensity = densityAtPoint(inScatterPoint);
        viewRayOpticalDepth += localDensity * stepSize;

        // Transmittance: how much light survives from sun → sample → camera
        vec3 transmittance = exp(-(sunRayOpticalDepth + viewRayOpticalDepth) * uScatteringCoefficients);

        // Accumulate scattered light weighted by density and step size
        inScatteredLight += localDensity * transmittance * stepSize;

        inScatterPoint += rayDir * stepSize;
    }

    // Apply phase function, scattering coefficients, and light intensity
    inScatteredLight *= phase * uScatteringCoefficients * uIntensity;

    // Dither to reduce banding
    inScatteredLight += blueNoise * 0.01;

    // Physically-based compositing: transmittance along view ray attenuates surface
    vec3 viewTransmittance = exp(-viewRayOpticalDepth * uScatteringCoefficients);
    return originalColor * viewTransmittance + inScatteredLight;
}

// Reconstruct world-space ray from screen UV
vec3 getViewRay(vec2 uv)
{
    // Convert UV to NDC (-1 to 1)
    vec2 ndc = uv * 2.0 - 1.0;

    // Unproject to view space
    vec4 viewSpace = uInvProjection * vec4(ndc, 0.0, 1.0);
    viewSpace.xyz /= viewSpace.w;

    // Transform to world space direction
    vec3 worldDir = (uInvView * vec4(viewSpace.xyz, 0.0)).xyz;
    return normalize(worldDir);
}

// Convert depth buffer value to linear depth
float linearizeDepth(float depth)
{
    // Standard depth buffer linearization
    return uNearPlane * uFarPlane / (uFarPlane - depth * (uFarPlane - uNearPlane));
}

void main()
{
    // Sample scene color and depth
    vec3 originalColor = texture(uSceneTexture, vUv).rgb;
    float depthSample = texture(uDepthTexture, vUv).r;

    // Reconstruct view ray
    vec3 rayDir = getViewRay(vUv);
    vec3 rayOrigin = uCameraPos;

    // Calculate scene depth in world units
    float linearDepth = linearizeDepth(depthSample);
    float sceneDepth = linearDepth / dot(rayDir, (uInvView * vec4(0.0, 0.0, -1.0, 0.0)).xyz);

    // Consider ocean surface as well
    float dstToOcean = raySphere(uPlanetCenter, uOceanRadius, rayOrigin, rayDir).x;
    float dstToSurface = min(sceneDepth, dstToOcean);

    // Ray-atmosphere intersection
    vec2 atmosphereHit = raySphere(uPlanetCenter, uAtmosphereRadius, rayOrigin, rayDir);
    float dstToAtmosphere = atmosphereHit.x;
    float dstThroughAtmosphere = min(atmosphereHit.y, dstToSurface - dstToAtmosphere);

    // Only calculate scattering if ray passes through atmosphere
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
