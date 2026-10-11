// Ground material textures (GroundTextures): one array layer per material, sampled triplanar on
// the planet-relative position at two incommensurate scales so the tile repeat does not show.
// Mips take care of distance: far away a layer averages to its mean, which the detail encoding
// maps to 1, so the shader's own material colour shows unchanged.

#ifndef GROUND_MATERIAL_GLSL
#define GROUND_MATERIAL_GLSL

layout(binding = 6) uniform sampler2DArray uGroundDetail; // rgb: 0.5 * albedo / mean, a: height
layout(binding = 7) uniform sampler2DArray uGroundNormal; // rgb: tangent normal * 0.5 + 0.5

const int GROUND_ROCK = 0;
const int GROUND_GRASS = 1;
const int GROUND_SAND = 2;
const int GROUND_SNOW = 3;
const int GROUND_SOIL = 4;

struct GroundSample
{
    vec3 albedo;  // multiplier around 1
    vec3 normal;  // world-space perturbation to add to the shading normal
};

// One scale, one layer, triplanar. Planes with a tiny weight are skipped.
GroundSample groundPlanar(vec3 p, vec3 w, float layer, float tile, vec2 offset)
{
    GroundSample s = GroundSample(vec3(0.0), vec3(0.0));
    vec3 q = p / tile;
    if (w.x > 0.02)
    {
        vec3 uv = vec3(q.yz + offset, layer);
        s.albedo += texture(uGroundDetail, uv).rgb * 2.0 * w.x;
        vec2 t = texture(uGroundNormal, uv).xy * 2.0 - 1.0;
        s.normal += vec3(0.0, t.x, t.y) * w.x;
    }
    if (w.y > 0.02)
    {
        vec3 uv = vec3(q.zx + offset.yx, layer);
        s.albedo += texture(uGroundDetail, uv).rgb * 2.0 * w.y;
        vec2 t = texture(uGroundNormal, uv).xy * 2.0 - 1.0;
        s.normal += vec3(t.y, 0.0, t.x) * w.y;
    }
    if (w.z > 0.02)
    {
        vec3 uv = vec3(q.xy + offset * 1.7, layer);
        s.albedo += texture(uGroundDetail, uv).rgb * 2.0 * w.z;
        vec2 t = texture(uGroundNormal, uv).xy * 2.0 - 1.0;
        s.normal += vec3(t.x, t.y, 0.0) * w.z;
    }
    float total = (w.x > 0.02 ? w.x : 0.0) + (w.y > 0.02 ? w.y : 0.0) + (w.z > 0.02 ? w.z : 0.0);
    s.albedo /= max(total, 1e-4);
    s.normal /= max(total, 1e-4);
    return s;
}

// How much of a tile scale this pixel should show: full while a tile spans many pixels, gone
// once it shrinks to a few, where its mips would only print the tile repeat as a grid.
float groundScaleWeight(float tile, float footprint)
{
    return 1.0 - smoothstep(tile / 96.0, tile / 24.0, footprint);
}

// Both scales of one material layer; footprint in matPos units per pixel. Each scale fades to
// neutral (albedo 1, no tilt) as it gets too small to read. tex = (fine tile, coarse tile, colour
// strength, normal strength); tiles are in matPos units (the planet scaled to radius 10). The fine
// tile puts about one texel on a pixel at the closest flying height; the coarse one breaks up the repeat.
GroundSample groundLayer(vec3 p, vec3 w, int layer, float footprint, vec4 tex)
{
    float fineW = groundScaleWeight(tex.x, footprint);
    float coarseW = groundScaleWeight(tex.y, footprint);
    GroundSample s = GroundSample(vec3(1.0), vec3(0.0));
    if (fineW > 0.0)
    {
        GroundSample fine = groundPlanar(p, w, float(layer), tex.x, vec2(0.0));
        s.albedo *= mix(vec3(1.0), fine.albedo, fineW * 0.7 * tex.z);
        s.normal += fine.normal * fineW * tex.w;
    }
    if (coarseW > 0.0)
    {
        GroundSample coarse = groundPlanar(p, w, float(layer), tex.y, vec2(0.31, 0.67));
        s.albedo *= mix(vec3(1.0), coarse.albedo, coarseW * 0.5 * tex.z);
        s.normal += coarse.normal * coarseW * 0.5 * tex.w;
    }
    return s;
}

// Triplanar blend weights, sharpened so the seams between projections stay narrow.
vec3 groundTriplanarWeights(vec3 n)
{
    vec3 w = pow(abs(n), vec3(4.0));
    return w / max(w.x + w.y + w.z, 1e-4);
}

#endif
