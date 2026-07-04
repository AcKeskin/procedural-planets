#pragma once

#include "Command.h"

#include <nlohmann/json_fwd.hpp>

#include <optional>
#include <string>

namespace planets::app::instrument
{

// Parse one command object (the {"cmd": ...} form). Returns false (with a [Script] warning)
// for an unknown or malformed verb so the caller can skip it. Note: setParams is NOT handled
// here — it expands to multiple SetParam commands in ParseTimeline. Shared with the live
// command source, which feeds one command object per message.
bool ParseCommand(const nlohmann::json& j, Command& out);

// Parse a timeline JSON document. On a structural error (not an object, no timeline array,
// an unrecognized command) returns nullopt and fills err. Per-field problems inside a
// command warn and fall back to defaults (CaptureRequest style), they don't fail the parse.
std::optional<Timeline> ParseTimeline(const std::string& json, std::string& err);

} // namespace planets::app::instrument
