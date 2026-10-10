#pragma once

#include "../settings/AtmosphereSettings.h"
#include "../settings/CloudSettings.h"
#include "../settings/OceanSettings.h"
#include "../settings/SceneSettings.h"
#include "../settings/SurfaceSettings.h"
#include <nlohmann/json_fwd.hpp>
#include <string>
#include <vector>

namespace planets::render::types
{

// How a planet type looks: the sky, clouds, fluid and star a body JSON asks for in its `render`
// block. Anything the block leaves out keeps the app default, so a body with no block looks like
// Earth always did.
struct BodyLook
{
    effects::AtmosphereSettings atmosphere;
    effects::CloudSettings clouds;
    effects::OceanSettings ocean;
    glm::vec3 sunColor = SceneSettings{}.sunColor;
    LightingSettings lighting; // only ambient, night lights and haze come from the block
    SurfaceSettings surface;
    // Biome colours in Whittaker order (tropical wet/mid/dry, temperate x3, boreal x3, tundra,
    // ice). Empty: the body's registry palette.
    std::vector<glm::vec3> palette;
};

// Parses the `render` block of a body JSON (missing or not an object: all defaults).
BodyLook ParseBodyLook(const nlohmann::json& body);

// Reads the body JSON file at `path` and parses its `render` block.
BodyLook LoadBodyLook(const std::string& path);

// Writes the look into the app settings. Leaves the light direction, exposure and bloom alone:
// those belong to the camera and the user, not to the planet.
void ApplyBodyLook(const BodyLook& look,
                   effects::AtmosphereSettings& atmosphere,
                   effects::CloudSettings& clouds,
                   effects::OceanSettings& ocean,
                   SceneSettings& scene,
                   SurfaceSettings& surface);

} // namespace planets::render::types
