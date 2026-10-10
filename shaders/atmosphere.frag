#version 450 core

#include "tonemap.glsl"
#include "sky_radiance.glsl"
#include "bloom.glsl"

in vec2 vUv;

out vec4 outColor;

uniform sampler2D uSceneTexture;
uniform sampler2D uDepthTexture;
uniform sampler2D uBlueNoise;
uniform sampler2D uCloudTexture; // half-res cloud march: rgb light, a transmittance
uniform int uCloudsOn;

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

// In-scatter over the ray plus the surface behind it, dimmed by the ray's own extinction.
vec3 calculateLight(vec3 rayOrigin, vec3 rayDir, float rayLength, vec3 originalColor, vec2 uv)
{
    vec2 screenSize = vec2(textureSize(uSceneTexture, 0));
    float blueNoise = texture(uBlueNoise, squareUV(uv, screenSize) * uDitherScale).r;
    blueNoise = (blueNoise - 0.5) * uDitherStrength;

    vec3 sunDir = -normalize(uLightDir); // toward the sun
    SkyScatter scatter = skyScatter(rayOrigin, rayDir, rayLength, sunDir, uNumInScatteringPoints, uNumOpticalDepthPoints);
    vec3 inScatteredLight = scatter.inScatter * uSunColor + blueNoise * 0.01;

    return originalColor * scatter.transmittance + inScatteredLight;
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
    // Bloom joins the scene before the composite, so the sun glow is reddened with the disc.
    vec3 originalColor = texture(uSceneTexture, vUv).rgb + bloom(vUv);
    // Clouds sit in front of what lies behind them; the haze below then covers both.
    if (uCloudsOn != 0)
    {
        // Four bilinear taps one half-res texel apart: smooths the march's per-pixel jitter
        // pattern and the stair steps of the half-res upsample.
        vec2 texel = 1.0 / vec2(textureSize(uCloudTexture, 0));
        vec4 cloud = 0.25 * (texture(uCloudTexture, vUv + vec2(-0.5, -0.5) * texel)
                           + texture(uCloudTexture, vUv + vec2( 0.5, -0.5) * texel)
                           + texture(uCloudTexture, vUv + vec2(-0.5,  0.5) * texel)
                           + texture(uCloudTexture, vUv + vec2( 0.5,  0.5) * texel));
        originalColor = originalColor * cloud.a + cloud.rgb;
    }
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
