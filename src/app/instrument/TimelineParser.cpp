#include "TimelineParser.h"

#include <nlohmann/json.hpp>

#include <iostream>

namespace planets::app::instrument
{
namespace
{
using nlohmann::json;

void Warn(const std::string& msg)
{
    std::cerr << "[Script] " << msg << std::endl;
}

bool ReadVec3(const json& j, glm::vec3& out)
{
    if (!j.is_array() || j.size() != 3)
        return false;
    for (int i = 0; i < 3; ++i)
    {
        if (!j[i].is_number())
            return false;
        out[i] = j[i].get<float>();
    }
    return true;
}

// A scalar param value from JSON: bool, integer, float, or a 3-array → vec3.
bool ReadParamValue(const json& j, ParamValue& out)
{
    if (j.is_boolean()) { out = j.get<bool>(); return true; }
    if (j.is_number_integer() || j.is_number_unsigned()) { out = j.get<int>(); return true; }
    if (j.is_number_float()) { out = j.get<float>(); return true; }
    if (j.is_array())
    {
        glm::vec3 v;
        if (ReadVec3(j, v)) { out = v; return true; }
    }
    return false;
}

PathEase ParseEase(const std::string& s)
{
    if (s == "linear") return PathEase::Linear;
    if (s == "smoothstep") return PathEase::Smoothstep;
    return PathEase::CatmullRom; // default + "catmull-rom"
}

bool ParseWaypoint(const json& j, Waypoint& wp)
{
    wp.t = j.value("t", 0.0f);
    if (j.contains("eye") || j.contains("target"))
    {
        wp.orbit = false;
        glm::vec3 e{0.0f}, tg{0.0f};
        if (j.contains("eye") && !ReadVec3(j["eye"], e)) return false;
        if (j.contains("target") && !ReadVec3(j["target"], tg)) return false;
        wp.eye = e;
        wp.target = tg;
        return true;
    }
    // orbit waypoint
    wp.orbit = true;
    wp.distance = j.value("distance", 0.0f);
    wp.distanceRadii = j.value("distanceRadii", 0.0f);
    wp.yaw = j.value("yaw", 0.0f);
    wp.pitch = j.value("pitch", 0.0f);
    return true;
}

bool ParseCamera(const json& j, Command& out)
{
    CameraCommand c;
    const std::string mode = j.value("mode", "lookat");
    glm::vec3 eye{0.0f}, target{0.0f};

    if (mode == "lookat" || mode == "lookAt")
    {
        c.mode = CameraMode::LookAt;
        if (j.contains("eye")) ReadVec3(j["eye"], c.eye);
        if (j.contains("target")) ReadVec3(j["target"], c.target);
    }
    else if (mode == "freefly")
    {
        c.mode = CameraMode::FreeFly;
        if (j.contains("eye")) ReadVec3(j["eye"], c.eye);
        c.yaw = j.value("yaw", -90.0f);
        c.pitch = j.value("pitch", 0.0f);
    }
    else if (mode == "orbit")
    {
        c.mode = CameraMode::Orbit;
        if (j.contains("target")) ReadVec3(j["target"], c.target);
        c.distance = j.value("distance", 0.0f);
        c.distanceRadii = j.value("distanceRadii", 0.0f);
        c.yaw = j.value("yaw", -90.0f);
        c.pitch = j.value("pitch", 0.0f);
    }
    else if (mode == "path")
    {
        c.mode = CameraMode::Path;
        c.ease = ParseEase(j.value("ease", "catmull-rom"));
        c.loop = j.value("loop", false);
        if (!j.contains("waypoints") || !j["waypoints"].is_array() || j["waypoints"].empty())
        {
            Warn("camera path has no waypoints; ignoring command");
            return false;
        }
        for (const auto& wj : j["waypoints"])
        {
            Waypoint wp;
            if (ParseWaypoint(wj, wp))
                c.waypoints.push_back(wp);
            else
                Warn("skipping malformed waypoint");
        }
        if (c.waypoints.empty())
            return false;
    }
    else
    {
        Warn("unknown camera mode '" + mode + "'; ignoring command");
        return false;
    }

    out.type = CommandType::Camera;
    out.payload = c;
    return true;
}

} // namespace

// Parse one command object. Returns false (with a warning) for an unknown/empty verb so the
// caller can skip it. Shared by the timeline loop and the live command source.
bool ParseCommand(const nlohmann::json& j, Command& out)
{
    if (!j.is_object() || !j.contains("cmd"))
    {
        Warn("command is not an object with a 'cmd' field; skipping");
        return false;
    }
    const std::string cmd = j["cmd"].get<std::string>();

    if (cmd == "setSeed")
    {
        SetSeedCommand c;
        c.seed = j.value("seed", 42u);
        out.type = CommandType::SetSeed;
        out.payload = c;
        return true;
    }
    if (cmd == "setBody")
    {
        SetBodyCommand c;
        c.body = j.value("body", "");
        c.path = j.value("path", "");
        if (c.body.empty() && c.path.empty())
        {
            Warn("setBody needs 'body' or 'path'; skipping");
            return false;
        }
        out.type = CommandType::SetBody;
        out.payload = c;
        return true;
    }
    if (cmd == "setParam")
    {
        SetParamCommand c;
        c.key = j.value("key", "");
        if (c.key.empty() || !j.contains("value") || !ReadParamValue(j["value"], c.value))
        {
            Warn("setParam needs a 'key' and a scalar/vec3 'value'; skipping");
            return false;
        }
        out.type = CommandType::SetParam;
        out.payload = c;
        return true;
    }
    if (cmd == "setParams")
    {
        // expanded by the caller into multiple SetParam commands; we don't support it
        // as a single Command, so reject here and let ParseTimeline handle expansion.
        Warn("setParams handled by timeline expansion, not ParseCommand");
        return false;
    }
    if (cmd == "camera")
        return ParseCamera(j, out);
    if (cmd == "wait")
    {
        WaitCommand c;
        if (j.contains("frames"))
            c.frames = j["frames"].get<int>();
        else if (j.contains("seconds"))
            c.frames = -1; // resolved against fps by the caller
        out.type = CommandType::Wait;
        out.payload = c;
        return true;
    }
    if (cmd == "capture")
    {
        CaptureCommand c;
        c.path = j.value("path", "");
        if (c.path.empty())
        {
            Warn("capture needs a 'path'; skipping");
            return false;
        }
        out.type = CommandType::Capture;
        out.payload = c;
        return true;
    }
    if (cmd == "burst")
    {
        BurstCommand c;
        c.frames = j.value("frames", 1);
        c.every = (std::max)(1, j.value("every", 1));
        c.pattern = j.value("pattern", "");
        if (c.pattern.empty())
        {
            Warn("burst needs a 'pattern'; skipping");
            return false;
        }
        out.type = CommandType::Burst;
        out.payload = c;
        return true;
    }

    Warn("unknown command '" + cmd + "'; skipping");
    return false;
}

std::optional<Timeline> ParseTimeline(const std::string& jsonText, std::string& err)
{
    json doc;
    try
    {
        doc = json::parse(jsonText, nullptr, true, /*ignore_comments=*/true);
    }
    catch (const json::parse_error& e)
    {
        err = std::string("JSON parse error: ") + e.what();
        return std::nullopt;
    }

    if (!doc.is_object())
    {
        err = "timeline root must be an object";
        return std::nullopt;
    }

    Timeline tl;

    if (doc.contains("setup") && doc["setup"].is_object())
    {
        const auto& s = doc["setup"];
        if (s.contains("seed")) { tl.seed = s["seed"].get<uint32_t>(); tl.hasSeed = true; }
        if (s.contains("body")) { tl.body = s["body"].get<std::string>(); tl.hasBody = true; }
        tl.width = s.value("width", 0);
        tl.height = s.value("height", 0);
        tl.fps = s.value("fps", 30.0f);
        if (tl.fps <= 0.0f)
            tl.fps = 30.0f;
    }

    if (!doc.contains("timeline") || !doc["timeline"].is_array())
    {
        err = "missing 'timeline' array";
        return std::nullopt;
    }

    const float dt = (tl.fps > 0.0f) ? (1.0f / tl.fps) : (1.0f / 30.0f);

    for (const auto& cj : doc["timeline"])
    {
        // setParams: expand to a SetParam per key/value pair, preserving order isn't
        // guaranteed across the object but order within a group doesn't matter (each is
        // independent), and the dirty-batching makes the regen cost identical.
        if (cj.is_object() && cj.value("cmd", "") == "setParams" && cj.contains("values") &&
            cj["values"].is_object())
        {
            for (const auto& [k, vj] : cj["values"].items())
            {
                SetParamCommand sp;
                sp.key = k;
                if (!ReadParamValue(vj, sp.value))
                {
                    Warn("setParams value for '" + k + "' is not scalar/vec3; skipping");
                    continue;
                }
                Command c;
                c.type = CommandType::SetParam;
                c.payload = sp;
                tl.commands.push_back(std::move(c));
            }
            continue;
        }

        Command c;
        if (!ParseCommand(cj, c))
            continue;

        // resolve seconds-based wait against fps
        if (c.type == CommandType::Wait)
        {
            auto& w = std::get<WaitCommand>(c.payload);
            if (w.frames < 0 && cj.contains("seconds"))
                w.frames = (std::max)(1, static_cast<int>(cj["seconds"].get<float>() / dt + 0.5f));
            if (w.frames < 0)
                w.frames = 1;
        }

        tl.commands.push_back(std::move(c));
    }

    return tl;
}

} // namespace planets::app::instrument
