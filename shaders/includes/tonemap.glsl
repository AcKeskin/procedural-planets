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
    // Near-neutral light over-exposed in every channel (sunlit cloud, snow, the high sun) burns to
    // white like film; a saturated highlight (a sunset, the low sun) keeps its hue.
    float dimmest = min(c.r, min(c.g, c.b));
    float neutral = smoothstep(0.4, 0.8, dimmest / peak);
    float burn = (1.0 - exp(-3.0 * max(dimmest - TONEMAP_KNEE, 0.0))) * neutral;
    return mix(c * (shoulder / peak), vec3(shoulder), burn);
}
