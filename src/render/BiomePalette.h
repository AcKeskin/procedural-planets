#pragma once

#include "GpuBuffer.h"
#include <glm/glm.hpp>
#include <string>
#include <vector>

namespace planets::render
{

// std140-aligned biome color entry for SSBO upload
struct BiomeEntry
{
    glm::vec3 color;
    float parameter; // Body-type-specific threshold or weight
};

// Data-driven biome color palette loaded from JSON
// Uploaded to GPU as an SSBO for shader access
class BiomePalette
{
public:
    BiomePalette() = default;

    static BiomePalette LoadFromJson(const std::string& path);

    void UploadToGpu(GpuBuffer<BiomeEntry>& buffer) const;

    const std::string& GetName() const { return _name; }
    const std::vector<BiomeEntry>& GetEntries() const { return _entries; }
    int GetEntryCount() const { return static_cast<int>(_entries.size()); }
    bool IsValid() const { return !_entries.empty(); }

    // Direct access for GUI editing
    std::vector<BiomeEntry>& GetEntries() { return _entries; }

private:
    std::string _name;
    std::vector<BiomeEntry> _entries;
};

} // namespace planets::render
