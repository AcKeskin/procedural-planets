#version 450 core

// Volumetric cloud layer, marched at half resolution. Output: rgb = light the clouds send toward
// the camera (HDR), a = how much of the scene behind still shows through. The atmosphere pass
// composites it before its own haze.

#include "cloud_layer.glsl"
#include "sky_radiance.glsl"

in vec2 vTexCoord;
out vec4 outColor;

uniform sampler2D uDepthTexture;
uniform sampler2D uBlueNoise;
uniform mat4 uInvView;
uniform mat4 uInvProjection;
uniform vec3 uCameraPos;
uniform float uNearPlane;
uniform float uFarPlane;
uniform vec3 uLightDir;   // points FROM the sun
uniform vec3 uSunColor;
uniform float uSunIntensity;
uniform int uViewSteps;
uniform float uFrameJitter;
uniform float uPixelAngle; // radians per pixel of this (half-res) target

const int LIGHT_STEPS = 5;
// Below 1 so sunlit tops keep their shading under the tonemap instead of burning to flat white.
const float CLOUD_ALBEDO = 0.78;
const float PI = 3.14159265;

// Henyey-Greenstein, normalized to average 1 over the sphere.
float hg(float cosTheta, float g)
{
    float g2 = g * g;
    return (1.0 - g2) / pow(1.0 + g2 - 2.0 * g * cosTheta, 1.5);
}

// Bright silver lining toward the sun plus some back scatter.
float cloudPhase(float cosTheta)
{
    return mix(hg(cosTheta, 0.6), hg(cosTheta, -0.25), 0.3);
}

// Returns (distance to the sphere, distance through it) or a miss.
vec2 sphereHit(float radius, vec3 origin, vec3 dir)
{
    return skyRaySphere(uCloudCenter, radius, origin, dir);
}

float linearDepth(float d)
{
    return uNearPlane * uFarPlane / (uNearPlane + d * (uFarPlane - uNearPlane));
}

// Segment of the ray inside the slab, up to the scene surface. x = start, y = end (y <= x: none).
vec2 slabSegment(vec3 origin, vec3 dir, float surfaceDistance)
{
    float r = length(origin - uCloudCenter);
    vec2 top = sphereHit(uCloudTopRadius, origin, dir);
    if (top.y <= 0.0)
        return vec2(0.0, -1.0);
    vec2 base = sphereHit(uCloudBaseRadius, origin, dir);
    bool hitsBase = base.y > 0.0;
    float start, end;
    if (r > uCloudTopRadius)
    {
        start = top.x;
        end = hitsBase ? base.x : top.x + top.y;
    }
    else if (r > uCloudBaseRadius)
    {
        start = 0.0;
        end = hitsBase && base.x > 0.0 ? base.x : top.y;
    }
    else
    {
        // Below the layer: from where the ray leaves the base sphere to where it leaves the top.
        start = base.x + base.y;
        end = top.y;
    }
    // Long grazing paths beyond this add nothing a viewer can see but cost steps.
    float maxLength = (uCloudTopRadius - uCloudBaseRadius) * 12.0;
    end = min(end, min(surfaceDistance, start + maxLength));
    return vec2(start, end);
}

// Optical depth toward the sun, a few growing steps, shape only.
float lightOpticalDepth(vec3 p, vec3 sunDir, float footprint)
{
    float thickness = uCloudTopRadius - uCloudBaseRadius;
    float step = thickness * 0.06;
    float depth = 0.0;
    vec3 q = p;
    for (int i = 0; i < LIGHT_STEPS; ++i)
    {
        q += sunDir * step;
        depth += cloudDensity(q, max(footprint, step), false) * step;
        step *= 1.7;
    }
    return depth;
}

// Light that reaches a sample through optical depth tau. Three scattering octaves (Wrenninge):
// each later octave loses less light and scatters more evenly, standing in for the light that
// bounces around inside the cloud. Without it the shadowed sides go black.
vec3 sunScatter(vec3 sunLight, float tau, float cosTheta)
{
    vec3 sum = vec3(0.0);
    float a = 1.0, b = 1.0, c = 1.0;
    for (int i = 0; i < 3; ++i)
    {
        float beer = exp(-tau * b) * mix(1.0, 1.0 - exp(-tau * 2.0 * b), 0.5);
        sum += sunLight * (a * beer * mix(1.0, cloudPhase(cosTheta), c) * CLOUD_ALBEDO);
        a *= 0.5;
        b *= 0.4;
        c *= 0.5;
    }
    return sum;
}

void main()
{
    float depthSample = texture(uDepthTexture, vTexCoord).r;
    vec2 ndc = vTexCoord * 2.0 - 1.0;
    vec4 viewPos = uInvProjection * vec4(ndc, 0.0, 1.0);
    viewPos.xyz /= viewPos.w;
    vec3 dir = normalize((uInvView * vec4(viewPos.xyz, 0.0)).xyz);
    vec3 forward = (uInvView * vec4(0.0, 0.0, -1.0, 0.0)).xyz;
    // Reversed-Z: 0 is the far plane, so sky pixels march to the end of the slab.
    float surfaceDistance = depthSample > 0.0 ? linearDepth(depthSample) / dot(dir, forward) : 1e30;

    vec2 segment = slabSegment(uCameraPos, dir, surfaceDistance);
    if (segment.y <= segment.x)
    {
        outColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    vec3 sunDir = -normalize(uLightDir);
    float cosTheta = dot(dir, sunDir);

    // Sunlight reaching the layer, reddened by the air above it; one estimate per ray.
    // Where the light estimate is taken: a fixed distance into the layer, so it does not jump
    // where the segment end jumps (rays that hit terrain vs rays that leave the layer).
    vec3 mid = uCameraPos + dir * min(segment.x + (uCloudTopRadius - uCloudBaseRadius), 0.5 * (segment.x + segment.y));
    float airToSun = skyRaySphere(uPlanetCenter, uAtmosphereRadius, mid, sunDir).y;
    vec3 sunLight = uSunColor * uSunIntensity * skyTransmittance(skyOpticalDepth(mid, sunDir, airToSun, 6));
    // Sky light from above, bluish, fading with the sun height.
    float sunHeight = dot(normalize(mid - uCloudCenter), sunDir);
    vec3 skyLight = (vec3(0.22, 0.30, 0.42) + sunLight * 0.2) * smoothstep(-0.15, 0.3, sunHeight);

    // Extinction per world unit: the density field is per planet radius.
    float sigmaScale = 1.0 / uCloudPlanetRadius;
    float stepLength = (segment.y - segment.x) / float(uViewSteps);
    vec2 noiseUv = gl_FragCoord.xy / vec2(textureSize(uBlueNoise, 0));
    float jitter = fract(texture(uBlueNoise, noiseUv).r + uFrameJitter);
    float t = segment.x + stepLength * jitter;

    vec3 light = vec3(0.0);
    float transmittance = 1.0;
    for (int i = 0; i < uViewSteps; ++i)
    {
        vec3 p = uCameraPos + dir * t;
        float footprint = t * uPixelAngle;
        float density = cloudDensity(p, footprint, true);
        if (density > 0.001)
        {
            float sigma = density * sigmaScale;
            float h = clamp((length(p - uCloudCenter) - uCloudBaseRadius) / (uCloudTopRadius - uCloudBaseRadius), 0.0, 1.0);
            float sunVisible = skySunVisibility(p, sunDir);
            float toSun = lightOpticalDepth(p, sunDir, footprint) * sigmaScale;
            vec3 inScatter = sunScatter(sunLight, toSun, cosTheta) * sunVisible + skyLight * mix(0.4, 1.0, h);
            // Energy-conserving step integration (Hillaire 2015).
            float stepTransmittance = exp(-sigma * stepLength);
            light += transmittance * inScatter * (1.0 - stepTransmittance);
            transmittance *= stepTransmittance;
            if (transmittance < 0.01)
                break;
        }
        t += stepLength;
    }
    outColor = vec4(light, transmittance);
}
