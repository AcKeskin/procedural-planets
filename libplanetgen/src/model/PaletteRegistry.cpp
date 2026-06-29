#include "PaletteRegistry.h"

namespace planetgen
{

namespace
{

// Built-in palette data embedded directly — mirrors data/palettes/*.json contents.
// These are the canonical sources; the JSON files are the authoritative text but we
// embed here so the registry has no filesystem dependency.

Palette MakeEarthPalette()
{
    Palette p;
    p.name = "Earth";
    p.entries = {
        { { 0.05f, 0.28f, 0.03f }, 0.0f },  // tropical_wet
        { { 0.28f, 0.40f, 0.12f }, 0.0f },  // tropical_mid
        { { 0.76f, 0.62f, 0.35f }, 0.0f },  // tropical_dry
        { { 0.12f, 0.35f, 0.08f }, 0.0f },  // temperate_wet
        { { 0.32f, 0.42f, 0.18f }, 0.0f },  // temperate_mid
        { { 0.65f, 0.55f, 0.30f }, 0.0f },  // temperate_dry
        { { 0.06f, 0.20f, 0.08f }, 0.0f },  // boreal_wet
        { { 0.22f, 0.28f, 0.16f }, 0.0f },  // boreal_mid
        { { 0.42f, 0.40f, 0.32f }, 0.0f },  // boreal_dry
        { { 0.48f, 0.50f, 0.40f }, 0.0f },  // tundra
        { { 0.92f, 0.94f, 0.97f }, 0.0f },  // ice
    };
    return p;
}

Palette MakeVolcanicPalette()
{
    Palette p;
    p.name = "Volcanic";
    p.entries = {
        { { 0.95f, 0.45f, 0.02f }, 0.0f  },  // molten_lava
        { { 0.80f, 0.20f, 0.01f }, 0.1f  },  // cooling_lava
        { { 0.45f, 0.08f, 0.02f }, 0.2f  },  // hot_rock
        { { 0.20f, 0.06f, 0.03f }, 0.35f },  // warm_basalt
        { { 0.10f, 0.08f, 0.06f }, 0.5f  },  // dark_basalt
        { { 0.06f, 0.05f, 0.05f }, 0.7f  },  // cooled_rock
        { { 0.04f, 0.03f, 0.03f }, 1.0f  },  // obsidian_peak
    };
    return p;
}

Palette MakeCrystallinePalette()
{
    Palette p;
    p.name = "Crystalline";
    p.entries = {
        { { 0.08f, 0.04f, 0.15f }, 0.0f  },  // deep_amethyst
        { { 0.15f, 0.08f, 0.30f }, 0.15f },  // crystal_base
        { { 0.25f, 0.30f, 0.55f }, 0.3f  },  // sapphire
        { { 0.40f, 0.55f, 0.70f }, 0.5f  },  // ice_crystal
        { { 0.60f, 0.75f, 0.85f }, 0.7f  },  // light_quartz
        { { 0.85f, 0.90f, 0.98f }, 0.85f },  // crystal_peak
        { { 0.95f, 0.92f, 1.00f }, 1.0f  },  // diamond_tip
    };
    return p;
}

} // namespace

PaletteRegistry::PaletteRegistry()
{
    RegisterBuiltins();
}

void PaletteRegistry::RegisterBuiltins()
{
    Register(IdEarth,       MakeEarthPalette());
    Register(IdVolcanic,    MakeVolcanicPalette());
    Register(IdCrystalline, MakeCrystallinePalette());
}

void PaletteRegistry::Register(const std::string& id, Palette palette)
{
    _map[id] = std::move(palette);
}

const Palette& PaletteRegistry::Resolve(const std::string& id) const
{
    auto it = _map.find(id);
    if (it == _map.end())
        return _empty;
    return it->second;
}

bool PaletteRegistry::Contains(const std::string& id) const
{
    return _map.count(id) > 0;
}

} // namespace planetgen
