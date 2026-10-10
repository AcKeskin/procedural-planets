#version 450 core

// Bloom bright pass: half-res, keeps only the light above the threshold.

in vec2 vTexCoord;
out vec4 fragColor;

uniform sampler2D uSceneTexture;

// Above sunlit cloud and snow (up to ~1.6 with the capture sun), below the sun disc and glints.
const float BLOOM_THRESHOLD = 2.0;

void main()
{
    // Four bilinear taps cover the 4x4 scene texels under this texel, so a bright line or a few
    // pixel sun cannot slip between samples.
    vec2 texel = 1.0 / vec2(textureSize(uSceneTexture, 0));
    vec3 c = 0.25 * (texture(uSceneTexture, vTexCoord + vec2(-texel.x, -texel.y)).rgb
                   + texture(uSceneTexture, vTexCoord + vec2( texel.x, -texel.y)).rgb
                   + texture(uSceneTexture, vTexCoord + vec2(-texel.x,  texel.y)).rgb
                   + texture(uSceneTexture, vTexCoord + vec2( texel.x,  texel.y)).rgb);

    // Scale by the share of the peak above the threshold, so the glow keeps the source's hue.
    float peak = max(c.r, max(c.g, c.b));
    fragColor = vec4(c * (max(peak - BLOOM_THRESHOLD, 0.0) / max(peak, 1e-4)), 1.0);
}
