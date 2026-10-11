#pragma once

#include "Command.h"

#include "../../render/settings/SceneSettings.h"
#include "../../render/settings/TerrainSettings.h"
#include "../../render/settings/OceanSettings.h"
#include "../../render/settings/AtmosphereSettings.h"
#include "../../render/settings/CloudSettings.h"
#include "../../render/settings/SurfaceSettings.h"

#include <planetgen/planetgen.hpp>

#include <string>

namespace planetgen
{
struct BodyConfig;
}

namespace planets::app::instrument
{

// A view onto the live, mutable settings the instrument can drive. Application fills this
// with references to its own members; ParamRegistry mutates through it, so the registry
// stays decoupled from Application and unit-testable in isolation. body may be null when no
// body is loaded yet (body.* keys are then rejected).
struct ParamTargets
{
    render::SceneSettings* scene = nullptr;
    render::effects::OceanSettings* ocean = nullptr;
    render::effects::AtmosphereSettings* atmosphere = nullptr;
    render::effects::CloudSettings* clouds = nullptr;
    render::LodConfig* lod = nullptr;
    render::GenerationConfig* gen = nullptr;
    float* seaLevel = nullptr;
    planetgen::BodyConfig* body = nullptr;
    render::SurfaceSettings* surface = nullptr;
};

// Result of applying a dotted-key parameter.
struct ApplyResult
{
    bool applied = false;    // key resolved and value coerced
    bool needsRegen = false; // the touched param requires a LOD/terrain regenerate
};

// Resolve a dotted key (e.g. "scene.lighting.sunIntensity", "body.shape.heightScale") and
// apply value to the matching field in targets. Unknown key or a value that can't coerce →
// applied=false. The needsRegen flag tells the caller whether to batch a regenerate before
// the next capture.
ApplyResult ApplyParam(const ParamTargets& targets, const std::string& key, const ParamValue& value);

// True if key is a known instrument parameter (independent of whether a body is loaded).
bool IsKnownParam(const std::string& key);

} // namespace planets::app::instrument
