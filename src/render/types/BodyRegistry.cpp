#include "BodyRegistry.h"

#include <nlohmann/json.hpp>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>

namespace planets::render::types
{

namespace
{
std::string ReadName(const std::filesystem::path& file, const std::string& fallback)
{
    std::ifstream in(file);
    const auto json = nlohmann::json::parse(in, nullptr, false);
    if (json.is_discarded())
    {
        std::cerr << "[BodyRegistry] Skipping name of " << file.string() << ": not valid JSON" << std::endl;
        return fallback;
    }
    if (json.contains("metadata") && json["metadata"].contains("name") && json["metadata"]["name"].is_string())
        return json["metadata"]["name"].get<std::string>();
    return fallback;
}
} // namespace

std::vector<BodyEntry> ScanBodies(const std::string& dir)
{
    std::vector<BodyEntry> bodies;
    std::error_code ec;
    for (const auto& item : std::filesystem::directory_iterator(dir, ec))
    {
        if (!item.is_regular_file() || item.path().extension() != ".json")
            continue;
        const std::string id = item.path().stem().string();
        // Forward slashes: these paths go into JSON scripts and logs.
        bodies.push_back(
            {id, ReadName(item.path(), id), (std::filesystem::path(dir) / item.path().filename()).generic_string()});
    }
    if (ec)
        std::cerr << "[BodyRegistry] Cannot read " << dir << ": " << ec.message() << std::endl;

    std::sort(bodies.begin(),
              bodies.end(),
              [](const BodyEntry& a, const BodyEntry& b)
              {
                  if ((a.id == "earth") != (b.id == "earth"))
                      return a.id == "earth";
                  return a.id < b.id;
              });
    return bodies;
}

int FindBody(const std::vector<BodyEntry>& bodies, const std::string& id)
{
    for (size_t i = 0; i < bodies.size(); ++i)
        if (bodies[i].id == id)
            return static_cast<int>(i);
    return -1;
}

} // namespace planets::render::types
