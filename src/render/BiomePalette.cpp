#include "BiomePalette.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>

namespace planets::render
{

BiomePalette BiomePalette::LoadFromJson(const std::string& path)
{
    BiomePalette palette;

    std::ifstream file(path);
    if (!file.is_open())
    {
        std::cerr << "[BiomePalette] Failed to open: " << path << std::endl;
        return palette;
    }

    try
    {
        nlohmann::json json;
        file >> json;

        palette._name = json.value("name", "unnamed");

        for (const auto& entry : json.at("entries"))
        {
            BiomeEntry biome;
            auto color = entry.at("color");
            biome.color = glm::vec3(color[0].get<float>(), color[1].get<float>(), color[2].get<float>());
            biome.parameter = entry.value("parameter", 0.0f);
            palette._entries.push_back(biome);
        }

        std::cout << "[BiomePalette] Loaded '" << palette._name << "' with " << palette._entries.size()
                  << " entries from " << path << std::endl;
    }
    catch (const nlohmann::json::exception& e)
    {
        std::cerr << "[BiomePalette] JSON parse error in " << path << ": " << e.what() << std::endl;
        palette._entries.clear();
    }

    return palette;
}

void BiomePalette::UploadToGpu(GpuBuffer<BiomeEntry>& buffer) const
{
    if (_entries.empty())
        return;

    buffer.Upload(_entries);
}

} // namespace planets::render
