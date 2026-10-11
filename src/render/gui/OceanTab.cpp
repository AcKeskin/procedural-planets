#include "OceanTab.h"
#include "Widgets.h"
#include "../BodyRuntime.h"
#include "../settings/OceanSettings.h"
#include "../settings/SurfaceSettings.h"
#include <imgui.h>

namespace planets::render
{

bool OceanTab::Draw(BodyRuntime& body, effects::OceanSettings& o, float& seaLevel, SurfaceSettings& s)
{
    using namespace ui;
    bool regen = false;

    Check("Ocean", o.enabled, "Draw the sea. Off = a dry world: basins render as land.");

    if (ImGui::CollapsingHeader("Sea Level", ImGuiTreeNodeFlags_DefaultOpen))
    {
        // The body config holds it too, so a regenerate and a saved config use the same level.
        if (Slider("Sea Level", seaLevel, 0.9f, 1.1f, "%.3f",
                   "Water surface as a multiple of radius. 1.0 = base radius. 0.9 to 1.1."))
            body.Config().metadata.seaLevel = seaLevel;
        regen |= Committed();
    }

    ImGui::BeginDisabled(!o.enabled);
    if (ImGui::CollapsingHeader("Water", ImGuiTreeNodeFlags_DefaultOpen))
    {
        Color("Shallow", o.shallowColor, "Water colour over the shallows.");
        Color("Deep", o.deepColor, "Water colour over deep ocean.");
        Slider("Clarity", o.clarity, 0.02f, 1.0f, "%.2f", "How far you see into the water. Low = murky.");
    }
    if (ImGui::CollapsingHeader("Waves"))
    {
        Slider("Wave Strength", o.waveStrength, 0.0f, 0.5f, "%.2f", "Steepness of the waves: rougher water, wider glitter.");
        Slider("Wave Frequency", o.waveScale, 0.25f, 4.0f, "%.2f",
               "Wave size relative to the planet. Higher = smaller waves.", ImGuiSliderFlags_Logarithmic);
        Slider("Wave Speed", o.waveSpeed, 0.1f, 2.0f, "%.1f", "How fast the waves move.");
    }
    if (ImGui::CollapsingHeader("Fluid"))
    {
        Slider("Glow", o.emissive, 0.0f, 5.0f, "%.2f", "Light the fluid gives off, for lava. 0 = none.");
        Slider("Crust", o.crust, 0.0f, 1.0f, "%.2f", "Share of the surface under dark floating crust.");
        Color("Crust Colour", o.crustColor, "Colour of the floating crust.");
        Slider("Frozen", o.frozen, 0.0f, 1.0f, "%.2f", "Ice sheet over the fluid. 1 = fully frozen.");
        Slider("Reflectivity", o.reflectivity, 0.0f, 2.0f, "%.2f", "Sky reflection and sun glint. Lava barely reflects.");
        Color("Foam Colour", o.foamColor, "Surf along the shore.");
    }
    ImGui::EndDisabled();

    if (ImGui::CollapsingHeader("Seabed Colour"))
    {
        Color("Shallow Floor", s.seafloor.shallowColor, "Ground colour just below sea level.");
        Color("Deep Floor", s.seafloor.deepColor, "Ground colour deep below sea level.");
        Slider("Depth Range", s.seafloor.depthRange, 0.005f, 0.3f, "%.3f", "Depth over which the floor turns deep.");
    }

    return regen;
}

} // namespace planets::render
