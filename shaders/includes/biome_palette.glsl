// Whittaker biome colour from the body palette (data/palettes/*.json, uploaded by the renderer at
// BiomePaletteBindingPoint): temperature picks the band, moisture the entry within it. Overlapping
// weights, so biome borders are soft gradients rather than lines.

#ifndef BIOME_PALETTE_GLSL
#define BIOME_PALETTE_GLSL

// Must match BiomePaletteBindingPoint in GpuConstants.h.
layout(std430, binding = 4) readonly buffer BiomePaletteBuffer
{
    vec4 paletteEntries[]; // xyz = colour, w = parameter
};

uniform int uPaletteSize;

// Entry order: 0-2 tropical (wet, mid, dry), 3-5 temperate, 6-8 boreal, 9 tundra, 10 ice.
vec3 paletteColor(int index, vec3 fallback)
{
    return uPaletteSize > index ? paletteEntries[index].xyz : fallback;
}

vec3 biomeByMoisture(float moisture, vec3 dry, vec3 mid, vec3 wet)
{
    float wDry = 1.0 - smoothstep(0.15, 0.50, moisture);
    float wMid = smoothstep(0.15, 0.45, moisture) * (1.0 - smoothstep(0.55, 0.85, moisture));
    float wWet = smoothstep(0.50, 0.80, moisture);
    return (dry * wDry + mid * wMid + wet * wWet) / (wDry + wMid + wWet);
}

struct Biome
{
    vec3 color;
    float polar; // weight of the tundra/ice band
};

Biome whittakerBiome(float temperature, float moisture)
{
    vec3 tropical = biomeByMoisture(moisture, paletteColor(2, vec3(0.76, 0.62, 0.35)),
                                    paletteColor(1, vec3(0.28, 0.40, 0.12)), paletteColor(0, vec3(0.05, 0.28, 0.03)));
    vec3 temperate = biomeByMoisture(moisture, paletteColor(5, vec3(0.65, 0.55, 0.30)),
                                     paletteColor(4, vec3(0.32, 0.42, 0.18)), paletteColor(3, vec3(0.12, 0.35, 0.08)));
    vec3 boreal = biomeByMoisture(moisture, paletteColor(8, vec3(0.42, 0.40, 0.32)),
                                  paletteColor(7, vec3(0.22, 0.28, 0.16)), paletteColor(6, vec3(0.06, 0.20, 0.08)));
    vec3 polar = mix(paletteColor(10, vec3(0.92, 0.94, 0.97)), paletteColor(9, vec3(0.48, 0.50, 0.40)),
                     smoothstep(0.0, 0.2, moisture));

    float wTropical  = smoothstep(0.50, 0.80, temperature);
    float wTemperate = smoothstep(0.25, 0.55, temperature) * (1.0 - smoothstep(0.60, 0.85, temperature));
    float wBoreal    = smoothstep(0.08, 0.30, temperature) * (1.0 - smoothstep(0.35, 0.60, temperature));
    float wPolar     = 1.0 - smoothstep(0.08, 0.30, temperature);
    float wSum = wTropical + wTemperate + wBoreal + wPolar;

    vec3 color = (tropical * wTropical + temperate * wTemperate + boreal * wBoreal + polar * wPolar) / wSum;
    return Biome(color, wPolar / wSum);
}

#endif
