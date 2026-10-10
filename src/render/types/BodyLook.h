#pragma once

#include "../settings/AtmosphereSettings.h"
#include "../settings/CloudSettings.h"
#include "../settings/OceanSettings.h"
#include "../settings/SceneSettings.h"
#include <nlohmann/json_fwd.hpp>
#include <string>

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
                   SceneSettings& scene);

} // namespace planets::render::types
