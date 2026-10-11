#include "Bookmarks.h"
#include <nlohmann/json.hpp>
#include <fstream>

namespace planets::app
{

namespace
{
nlohmann::json ToJson(const glm::vec3& v)
{
    return nlohmann::json::array({v.x, v.y, v.z});
}

bool ReadVec3(const nlohmann::json& j, const char* key, glm::vec3& out)
{
    const auto it = j.find(key);
    if (it == j.end() || !it->is_array() || it->size() != 3)
        return false;
    out = glm::vec3((*it)[0].get<float>(), (*it)[1].get<float>(), (*it)[2].get<float>());
    return true;
}
} // namespace

void Bookmarks::Set(int slot, const Bookmark& bookmark)
{
    if (slot >= 1 && slot <= SlotCount)
        _slots[slot - 1] = bookmark;
}

const Bookmark& Bookmarks::Get(int slot) const
{
    static const Bookmark empty;
    return slot >= 1 && slot <= SlotCount ? _slots[slot - 1] : empty;
}

void Bookmarks::Load(const std::string& path)
{
    std::ifstream in(path);
    const nlohmann::json root = nlohmann::json::parse(in, nullptr, false);
    if (!root.is_object() || !root.contains("bookmarks") || !root["bookmarks"].is_array())
        return;

    for (const nlohmann::json& entry : root["bookmarks"])
    {
        if (!entry.is_object() || !entry.contains("slot") || !entry["slot"].is_number_integer())
            continue;
        Bookmark b;
        if (!ReadVec3(entry, "position", b.pose.position) || !ReadVec3(entry, "forward", b.pose.forward) ||
            !ReadVec3(entry, "up", b.pose.up))
            continue;
        ReadVec3(entry, "sunDir", b.sunDir);
        b.pose.mode = entry.value("mode", "orbit") == "fly" ? core::CameraMode::FreeFly : core::CameraMode::Orbit;
        b.used = true;
        Set(entry["slot"].get<int>(), b);
    }
}

bool Bookmarks::Save(const std::string& path) const
{
    nlohmann::json list = nlohmann::json::array();
    for (int slot = 1; slot <= SlotCount; ++slot)
    {
        const Bookmark& b = Get(slot);
        if (!b.used)
            continue;
        list.push_back({{"slot", slot},
                        {"mode", b.pose.mode == core::CameraMode::Orbit ? "orbit" : "fly"},
                        {"position", ToJson(b.pose.position)},
                        {"forward", ToJson(b.pose.forward)},
                        {"up", ToJson(b.pose.up)},
                        {"sunDir", ToJson(b.sunDir)}});
    }
    std::ofstream out(path);
    if (!out)
        return false;
    out << nlohmann::json{{"bookmarks", list}}.dump(2) << '\n';
    return static_cast<bool>(out);
}

} // namespace planets::app
