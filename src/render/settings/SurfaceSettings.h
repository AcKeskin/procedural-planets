#pragma once

#include <glm/glm.hpp>

namespace planets::render
{

// Every rule of the shared surface shader, set per body from its `render.surface` block. The defaults
// are the values the shader used as constants, so a body without the block looks like Earth always did.
// Ranges are (start, end) of a smoothstep.
struct SurfaceSettings
{
    struct Rock
    {
        glm::vec3 colorA = glm::vec3(0.30f, 0.29f, 0.27f); // dark end of the grain
        glm::vec3 colorB = glm::vec3(0.42f, 0.40f, 0.37f); // light end of the grain
        float roughnessA = 0.95f;
        float roughnessB = 0.7f;
        glm::vec3 streakColor = glm::vec3(0.34f, 0.26f, 0.18f); // iron/ochre streaks
        float streakAmount = 0.5f;
        glm::vec3 tint = glm::vec3(1.0f);
        float slope = 0.35f;      // steepness where rock takes over
        float slopeNoise = 0.25f; // how much that threshold wanders
        float softness = 0.12f;   // half-width of the rock edge
    };

    struct Sand
    {
        glm::vec3 colorA = glm::vec3(0.62f, 0.52f, 0.36f);
        glm::vec3 colorB = glm::vec3(0.74f, 0.66f, 0.46f);
        float roughness = 0.9f;
        glm::vec3 tint = glm::vec3(1.0f);
        float cover = 0.0f;                                 // 1 = sand on every flat (dune seas)
        glm::vec2 aridity = glm::vec2(0.35f, 0.65f);        // dryness where beaches and deserts turn to sand
        glm::vec2 beachHeight = glm::vec2(0.12f, 0.25f);    // sand fades out above this height
        glm::vec2 desertAridity = glm::vec2(0.45f, 0.75f);  // dryness where the sand texture takes over
    };

    struct Snow
    {
        glm::vec3 colorA = glm::vec3(0.86f, 0.88f, 0.93f);
        glm::vec3 colorB = glm::vec3(0.97f, 0.98f, 1.0f);
        float roughness = 0.4f;
        glm::vec3 tint = glm::vec3(1.0f);
        float coldLowering = 0.04f; // how far cold climates pull the snow line down
        float blendBelow = 0.20f;   // snow fades in this far below the line
        float blendAbove = 0.05f;   // and is full this far above it
        float polarTemperature = 0.04f; // temperature below which the poles freeze
        float polarAmount = 0.5f;
        glm::vec2 slopeShed = glm::vec2(0.55f, 0.85f); // steepness where snow slides off
        float shedAmount = 0.7f;
    };

    struct Ground
    {
        glm::vec2 lightness = glm::vec2(0.8f, 1.15f); // patchy lightness of the biome colour
        float roughness = 0.85f;
        float vegetation = 1.0f;                      // 0 = no grass texture (bare soil)
        glm::vec2 soilDryness = glm::vec2(0.6f, 0.85f); // dryness where soil replaces grass
        float soilAmount = 0.6f;
        float polarSoil = 0.7f;
    };

    struct Detail
    {
        glm::vec2 grain = glm::vec2(0.72f, 1.28f); // fine albedo grain range
        float grainSnowFade = 0.7f;
        float mediumFrequency = 0.9f; // patch-scale variation
        float fineFrequency = 3.0f;   // grain
        float bumpFrequency = 45.0f;
        float bumpTilt = 0.012f;
        float bumpSnowFade = 0.6f;
        float textureBump = 0.6f;     // tilt from the material normal maps
        float roughnessFloor = 0.85f;      // ground stays near-matte
        float matteRoughnessFloor = 0.4f;  // rock and snow keep a little glint
        float gamma = 0.9f;
        float temperatureNoise = 0.06f;    // noise on biome borders
        float moistureNoise = 0.18f;
        float moistureDetailNoise = 0.12f;
    };

    // One ground texture layer (rock, grass, sand, snow, soil).
    struct Texture
    {
        float fineTile = 0.07f;   // tile size in radius-10 units
        float coarseTile = 0.37f;
        float strength = 1.0f;       // how much the texture changes the colour
        float normalStrength = 1.0f; // how much it tilts the normal
    };

    struct Seafloor
    {
        glm::vec3 shallowColor = glm::vec3(0.10f, 0.38f, 0.40f);
        glm::vec3 deepColor = glm::vec3(0.015f, 0.05f, 0.13f);
        float depthRange = 0.08f;
    };

    struct Climate
    {
        float polarCaps = 0.0f;    // share of each hemisphere under ice, from the pole (0.2 = small caps)
        float substellar = 0.0f;   // 1 = tidally locked: hot under the sun, frozen on the night side
        float cityCoverage = 0.0f; // how much land is built up (city lights)
    };

    // Rock layers: colour bands by height on steep rock (cliffs, crater walls). Strength 0 = off.
    struct Strata
    {
        float strength = 0.0f; // how much the bands replace the rock colour
        float bands = 40.0f; // bands over the land height range
        float warp = 0.5f; // how far the layers bend, in bands
        glm::vec3 colorA = glm::vec3(0.52f, 0.44f, 0.34f); // light layer
        glm::vec3 colorB = glm::vec3(0.3f, 0.26f, 0.22f); // dark layer
    };

    // Wet ground: darker and smoother near sea level and on the wettest land. Strength 0 = off.
    struct Wetness
    {
        float strength = 0.0f; // how wet the wet ground gets
        float shoreWidth = 0.06f; // height above sea level the shore stays wet
        float moisture = 0.35f; // moisture above which inland ground is wet
        float darkening = 0.4f; // how much wet ground darkens
        float roughness = 0.35f; // roughness of wet ground
        glm::vec3 tint = glm::vec3(1.0f, 1.0f, 1.0f); // colour cast of the wet ground (acid stains)
    };

    // Fill on low, flat ground (crater floors, basins, the foot of slopes). Strength 0 = off.
    struct Sediment
    {
        float strength = 0.0f; // how much fill covers the ground
        glm::vec3 color = glm::vec3(0.55f, 0.48f, 0.38f); // colour of the fill
        glm::vec2 height = glm::vec2(0.05f, 0.3f); // full below x, gone above y (land height 0-1)
        glm::vec2 slope = glm::vec2(0.02f, 0.1f); // full below x steepness, gone above y
        float patchiness = 0.5f; // 0 = even sheet, 1 = broken patches
    };

    // Vegetation thins on steep ground and above a treeline, showing bare ground. Strength 0 = off.
    struct Vegetation
    {
        float strength = 0.0f; // how fully slope and altitude strip the vegetation
        glm::vec2 slope = glm::vec2(0.08f, 0.22f); // steepness range where vegetation thins out
        glm::vec2 treeline = glm::vec2(0.45f, 0.7f); // land height range where vegetation thins out
        glm::vec3 bareColor = glm::vec3(0.42f, 0.37f, 0.3f); // colour of the bare ground
    };

    // One cover layer on top of everything: volcanic ash, frost or wind-blown dust. Strength 0 = off.
    struct Cover
    {
        float strength = 0.0f; // how much the cover hides the ground
        glm::vec3 color = glm::vec3(0.25f, 0.24f, 0.23f); // colour of the cover
        float roughness = 0.9f; // roughness of the cover
        glm::vec2 slope = glm::vec2(0.25f, 0.5f); // steepness range where the cover slides off
        glm::vec2 height = glm::vec2(0.0f, 1.0f); // land height range the cover lies in
        float facing = 0.0f; // +1 on slopes facing the sun, -1 on shaded slopes, 0 everywhere
        float sunFollow = 0.0f; // +1 toward the point under the sun, -1 toward the night side
        float patchiness = 0.4f; // 0 = even sheet, 1 = broken patches
    };

    static constexpr int TextureCount = 5; // order matches GROUND_* in ground_material.glsl

    Rock rock;
    Sand sand;
    Snow snow;
    Ground ground;
    Detail detail;
    Texture textures[TextureCount];
    Seafloor seafloor;
    Climate climate;
    Strata strata;
    Wetness wetness;
    Sediment sediment;
    Vegetation vegetation;
    Cover cover;
};

} // namespace planets::render
