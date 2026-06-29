#pragma once

// Id-keyed palette registry with built-in palettes (earth/volcanic/crystalline)
// embedded at init. No filesystem access in the contract — consumers call Register()
// with pre-loaded data; the registry does lookups only.

#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <unordered_map>

namespace planetgen
{

// Matches the layout of render::BiomeEntry — color + body-specific threshold/weight.
struct PaletteEntry
{
    glm::vec3 color     = {};
    float     parameter = 0.0f;
};

struct Palette
{
    std::string              name;
    std::vector<PaletteEntry> entries;

    bool IsValid() const { return !entries.empty(); }
};

class PaletteRegistry
{
public:
    PaletteRegistry();

    // Register (or overwrite) a palette by id.
    void Register(const std::string& id, Palette palette);

    // Look up by id; returns empty Palette if not found.
    const Palette& Resolve(const std::string& id) const;

    bool Contains(const std::string& id) const;

    // Built-in ids guaranteed to be present after construction.
    static constexpr const char* IdEarth       = "earth";
    static constexpr const char* IdVolcanic    = "volcanic";
    static constexpr const char* IdCrystalline = "crystalline";

private:
    void RegisterBuiltins();

    std::unordered_map<std::string, Palette> _map;
    Palette _empty; // returned for unknown ids
};

} // namespace planetgen
