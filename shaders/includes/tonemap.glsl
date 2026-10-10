// Display transform for the HDR scene target. The scene is authored in display space (no linear
// workflow), so a full filmic curve (ACES, AgX) would shift every tuned colour. Instead the curve
// is the identity up to a knee and rolls highlights (sun, limb, glints) smoothly into 1: LDR
// content looks as before, HDR content stops clipping and keeps its hue.

uniform float uExposure;

const float TONEMAP_KNEE = 0.8;

vec3 tonemap(vec3 hdr)
{
    // The shoulder acts on the brightest channel and scales the colour with it, so a bright orange
    // sunset stays orange instead of clipping toward yellow.
    vec3 c = max(hdr * uExposure, vec3(0.0));
    float peak = max(c.r, max(c.g, c.b));
    if (peak <= TONEMAP_KNEE)
        return c;
    float shoulder = TONEMAP_KNEE + (1.0 - TONEMAP_KNEE) * (1.0 - exp(-(peak - TONEMAP_KNEE) / (1.0 - TONEMAP_KNEE)));
    return c * (shoulder / peak);
}
