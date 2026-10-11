#pragma once

#include "../settings/AtmosphereSettings.h"
#include "../settings/CloudSettings.h"
#include "../settings/OceanSettings.h"
#include "../settings/SceneSettings.h"
#include "../settings/SurfaceSettings.h"
#include "planetgen/model/BodyConfig.h"
#include <nlohmann/json_fwd.hpp>
#include <cstdint>
#include <string>
#include <vector>

namespace planets::render::types
{

// The look tabs of the Planet window. Each one can be locked, randomized or reset on its own.
enum class LookUnit : uint32_t
{
    Terrain,
    Surface,
    Ocean,
    Sky,
    Light,
    Count
};

constexpr uint32_t UnitBit(LookUnit u)
{
    return 1u << static_cast<uint32_t>(u);
}
constexpr uint32_t AllUnits = (1u << static_cast<uint32_t>(LookUnit::Count)) - 1u;

// Every value the look tabs edit, in the structs the app renders with.
struct LookState
{
    planetgen::BodyConfig config;
    effects::AtmosphereSettings atmosphere;
    effects::CloudSettings clouds;
    effects::OceanSettings ocean;
    SceneSettings scene;
    SurfaceSettings surface;
    std::vector<glm::vec3> palette; // empty: the body's registry palette
};

// Copies the values of the units in `mask` from one state to the other. The split follows the
// tabs: the seabed colours belong to Ocean, the shadow fill to Light, the simple haze to Sky.
void CopyUnits(uint32_t mask, const LookState& from, LookState& to);

// The unit a variety key ("render.clouds.coverage") changes.
LookUnit UnitOfKey(const std::string& key);

// The units the body's variety block rolls; a unit outside it is the same at every seed.
uint32_t VarietyUnits(const nlohmann::json& body);

// The body's world at `seed`. Sun direction, exposure and the other values a body does not set
// come from `scene`. Returns an empty string on success, else the reason.
std::string RollLook(const nlohmann::json& body, uint32_t seed, const SceneSettings& scene, LookState& out);

} // namespace planets::render::types
