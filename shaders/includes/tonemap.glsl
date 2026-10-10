// Display transform for the HDR scene target. The scene is authored in display space (no linear
// workflow), so a full filmic curve (ACES, AgX) would shift every tuned colour. Instead the curve
// is the identity up to a knee and rolls highlights (sun, limb, glints) smoothly into 1: LDR
// content looks as before, HDR content stops clipping.

uniform float uExposure;

const float TONEMAP_KNEE = 0.8;

vec3 tonemap(vec3 hdr)
{
    vec3 c = max(hdr * uExposure, vec3(0.0));
    vec3 over = max(c - TONEMAP_KNEE, vec3(0.0));
    vec3 shoulder = TONEMAP_KNEE + (1.0 - TONEMAP_KNEE) * (1.0 - exp(-over / (1.0 - TONEMAP_KNEE)));
    return mix(c, shoulder, step(TONEMAP_KNEE, c));
}
