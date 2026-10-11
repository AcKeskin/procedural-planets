// Per-body surface rules (SurfaceSettings, packed by PackSurfaceBlock in SurfaceBlock.h). Every row is a
// vec4 so the layout matches C++ without std140 padding rules.

#ifndef SURFACE_PARAMS_GLSL
#define SURFACE_PARAMS_GLSL

layout(std140, binding = 5) uniform SurfaceBlock
{
    vec4 sRockColorA;      // rgb dark grain end, w roughness
    vec4 sRockColorB;      // rgb light grain end, w roughness
    vec4 sRockStreak;      // rgb streak colour, w amount
    vec4 sRockTint;        // rgb
    vec4 sSandColorA;      // rgb, w roughness
    vec4 sSandColorB;      // rgb
    vec4 sSandTint;        // rgb, w sand cover
    vec4 sSnowColorA;      // rgb, w roughness
    vec4 sSnowColorB;      // rgb
    vec4 sSnowTint;        // rgb
    vec4 sGround;          // xy lightness range, z roughness, w vegetation
    vec4 sRockRule;        // x slope, y slope noise, z softness
    vec4 sSnowRule0;       // x cold lowering, y blend below, z blend above, w polar temperature
    vec4 sSnowRule1;       // x polar amount, yz slope shed range, w shed amount
    vec4 sSandRule;        // xy aridity range, zw beach height range
    vec4 sSoilRule;        // xy desert aridity range, zw soil dryness range
    vec4 sSoilRule2;       // x soil amount, y polar soil
    vec4 sDetail0;         // xy grain range, z grain snow fade, w medium frequency
    vec4 sDetail1;         // x bump frequency, y bump tilt, z bump snow fade, w texture bump
    vec4 sDetail2;         // x roughness floor, y matte roughness floor, z gamma, w fine frequency
    vec4 sDetail3;         // x temperature noise, y moisture noise, z moisture detail noise
    vec4 sSeafloorShallow; // rgb, w depth range
    vec4 sSeafloorDeep;    // rgb
    vec4 sClimate;         // x polar caps, y substellar, z city coverage
    vec4 sTexture[5];      // per ground layer: x fine tile, y coarse tile, z strength, w normal strength
};

#endif
