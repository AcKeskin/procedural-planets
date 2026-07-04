// Shared cloud coverage field — used by the cloud shell (to draw) AND the surface shaders (to cast
// the matching soft shadow). One source so the shadow always lines up with the visible clouds.

#ifndef CLOUDS_GLSL
#define CLOUDS_GLSL

float cloudHash(vec3 p)
{
    p = fract(p * 0.3183099 + 0.1);
    p *= 17.0;
    return fract(p.x * p.y * p.z * (p.x + p.y + p.z));
}

float cloudValueNoise(vec3 p)
{
    vec3 i = floor(p);
    vec3 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    return mix(mix(mix(cloudHash(i + vec3(0, 0, 0)), cloudHash(i + vec3(1, 0, 0)), f.x),
                   mix(cloudHash(i + vec3(0, 1, 0)), cloudHash(i + vec3(1, 1, 0)), f.x), f.y),
               mix(mix(cloudHash(i + vec3(0, 0, 1)), cloudHash(i + vec3(1, 0, 1)), f.x),
                   mix(cloudHash(i + vec3(0, 1, 1)), cloudHash(i + vec3(1, 1, 1)), f.x), f.y), f.z);
}

float cloudFbm(vec3 p)
{
    float total = 0.0, amp = 0.5, freq = 1.0;
    for (int i = 0; i < 5; ++i)
    {
        total += cloudValueNoise(p * freq) * amp;
        freq *= 2.0;
        amp *= 0.5;
    }
    return total;
}

// Cloud opacity in [0,1] for a direction on the sphere. dir = unit direction from planet center;
// the scale/coverage/speed/time must match the values the cloud shell renders with, or the shadow
// drifts out of sync with the clouds.
float cloudCoverage(vec3 dir, float scale, float coverage, float speed, float time)
{
    vec3 samplePos = dir * scale + vec3(time * speed);
    float density = clamp((cloudFbm(samplePos) - 0.25) / 0.5, 0.0, 1.0);
    float threshold = 1.0 - coverage;
    return smoothstep(threshold, threshold + 0.25, density);
}

#endif // CLOUDS_GLSL
