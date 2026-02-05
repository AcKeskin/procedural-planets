// Math utilities for procedural planet generation
// Shared functions for ray intersection, smooth blending, and value remapping

#ifndef MATH_GLSL
#define MATH_GLSL

const float PI = 3.14159265359;
const float TAU = PI * 2.0;
const float MAX_FLOAT = 3.402823466e+38;

// Ray-sphere intersection using quadratic formula
// Returns vec2(distanceToSphere, distanceThroughSphere)
// If ray origin is inside sphere, distanceToSphere = 0
// If ray misses sphere, returns vec2(MAX_FLOAT, 0)
vec2 raySphere(vec3 sphereCenter, float sphereRadius, vec3 rayOrigin, vec3 rayDir) {
    vec3 offset = rayOrigin - sphereCenter;
    float a = 1.0; // Assumes rayDir is normalized
    float b = 2.0 * dot(offset, rayDir);
    float c = dot(offset, offset) - sphereRadius * sphereRadius;
    float discriminant = b * b - 4.0 * a * c;

    if (discriminant > 0.0) {
        float s = sqrt(discriminant);
        float dstToSphereNear = max(0.0, (-b - s) / (2.0 * a));
        float dstToSphereFar = (-b + s) / (2.0 * a);

        if (dstToSphereFar >= 0.0) {
            return vec2(dstToSphereNear, dstToSphereFar - dstToSphereNear);
        }
    }

    return vec2(MAX_FLOAT, 0.0);
}

// Smooth maximum using polynomial blending (Inigo Quilez)
// k controls transition sharpness; k = 0 behaves as max(a, b)
float smoothMax(float a, float b, float k) {
    k = min(0.0, -k);
    float h = clamp((b - a + k) / (2.0 * k), 0.0, 1.0);
    return a * h + b * (1.0 - h) - k * h * (1.0 - h);
}

// Smooth minimum using polynomial blending (Inigo Quilez)
// k controls transition sharpness; k = 0 behaves as min(a, b)
float smoothMin(float a, float b, float k) {
    k = max(0.0, k);
    float h = clamp((b - a + k) / (2.0 * k), 0.0, 1.0);
    return a * h + b * (1.0 - h) - k * h * (1.0 - h);
}

// Smoothstep-based transition for edge blending
// Returns 0-1 weight centered on edge with given width
float blend(float edge, float width, float value) {
    return smoothstep(edge - width * 0.5, edge + width * 0.5, value);
}

// Remap value from [inMin, inMax] to [outMin, outMax]
float remap(float value, float inMin, float inMax, float outMin, float outMax) {
    return clamp(outMin + (value - inMin) * (outMax - outMin) / (inMax - inMin), outMin, outMax);
}

// Remap value from [inMin, inMax] to [0, 1]
float remap01(float value, float inMin, float inMax) {
    return clamp((value - inMin) / (inMax - inMin), 0.0, 1.0);
}

#endif // MATH_GLSL
