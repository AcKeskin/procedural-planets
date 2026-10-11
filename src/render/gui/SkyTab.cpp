#include "SkyTab.h"
#include "Widgets.h"
#include "../settings/AtmosphereSettings.h"
#include "../settings/CloudSettings.h"
#include "../settings/SceneSettings.h"
#include <imgui.h>

namespace planets::render
{

void SkyTab::Draw(effects::AtmosphereSettings& a, effects::CloudSettings& c, LightingSettings& lighting,
                  bool bodyHasAir)
{
    using namespace ui;

    if (!bodyHasAir)
        ImGui::TextDisabled("This planet has no atmosphere.");
    ImGui::BeginDisabled(!bodyHasAir);

    if (ImGui::CollapsingHeader("Atmosphere", ImGuiTreeNodeFlags_DefaultOpen))
    {
        Check("Atmosphere (H)", a.enabled, "Scattering sky and its clouds. Key: H.");
        ImGui::BeginDisabled(!a.enabled);
        Slider("Height", a.atmosphereScale, 0.05f, 0.5f, "%.3f", "Atmosphere thickness as a fraction of radius. 0.05 to 0.5.");
        Slider("Air Density", a.scatteringStrength, 2.0f, 40.0f, "%.1f",
               "Thicker air: deeper sky colour and a brighter rim. 2 to 40.");
        Slider("Brightness", a.intensity, 0.5f, 3.0f, "%.2f", "Overall brightness of the scattered light.");
        Slider("Air Falloff", a.densityFalloff, 1.0f, 10.0f, "%.1f", "How fast the air thins with height.");
        if (ImGui::TreeNode("Sky Colour"))
        {
            const char* tip = "Wavelength in nm. Shorter scatters more. Earth: 680 / 550 / 440.";
            Slider("Red", a.wavelengths.x, 400.0f, 740.0f, "%.0f nm", tip);
            Slider("Green", a.wavelengths.y, 400.0f, 740.0f, "%.0f nm", tip);
            Slider("Blue", a.wavelengths.z, 400.0f, 740.0f, "%.0f nm", tip);
            ImGui::TreePop();
        }
        if (ImGui::TreeNode("Quality##sky"))
        {
            Slider("Sky Samples", a.numInScatteringPoints, 3, 20, "Steps along each view ray. More = smoother, slower.");
            Slider("Light Samples", a.numOpticalDepthPoints, 3, 20, "Steps toward the sun per sample. More = slower.");
            Slider("Dither", a.ditherStrength, 0.0f, 2.0f, "%.2f", "Noise that hides colour banding in the sky.");
            Slider("Dither Scale", a.ditherScale, 1.0f, 10.0f, "%.1f", "Size of the dither pattern.");
            ImGui::TreePop();
        }
        ImGui::EndDisabled();
    }

    if (ImGui::CollapsingHeader("Haze", ImGuiTreeNodeFlags_DefaultOpen))
    {
        if (a.enabled)
        {
            Slider("Haze", a.mieCoefficient, 0.0f, 15.0f, "%.2f",
                   "Dims the far surface and widens the glow around the sun.", ImGuiSliderFlags_Logarithmic);
            Slider("Haze Falloff", a.mieDensityFalloff, 1.0f, 60.0f, "%.1f",
                   "How fast haze thins with height. High = low ground fog.");
            Color("Haze Colour", a.hazeColor, "Colour the haze scatters: white air, ochre dust, green spores.");
        }
        else
        {
            // Without the scattering pass the surface shader adds a cheap distance haze instead.
            ImGui::TextDisabled("Atmosphere off: simple distance haze.");
            Slider("Haze##simple", lighting.hazeStrength, 0.0f, 0.4f, "%.2f", "Fades the far surface toward the haze colour.");
            Color("Haze Colour##simple", lighting.hazeColor, "Colour the far surface fades to.");
        }
    }

    if (ImGui::CollapsingHeader("Clouds", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::BeginDisabled(!a.enabled);
        Check("Clouds", c.enabled, a.enabled ? "Show the cloud layer." : "Clouds need the atmosphere.");
        ImGui::BeginDisabled(!c.enabled);
        Slider("Coverage", c.coverage, 0.0f, 1.0f, "%.2f", "Share of the sky with clouds. 0 to 1.");
        Slider("Density##clouds", c.density, 20.0f, 2000.0f, "%.0f", "How opaque each cloud is. 20 to 2000.",
               ImGuiSliderFlags_Logarithmic);
        Slider("Cloud Frequency", c.scale, 0.25f, 4.0f, "%.2f", "Higher = more, smaller clouds.",
               ImGuiSliderFlags_Logarithmic);
        Slider("Base Altitude", c.altitude, 0.0f, 0.1f, "%.3f", "Cloud floor height as a fraction of radius. 0 to 0.1.");
        Slider("Thickness", c.thickness, 0.005f, 0.08f, "%.3f", "Cloud layer depth as a fraction of radius.");
        Slider("Drift Speed", c.speed, 0.0f, 1.0f, "%.2f", "How fast the weather moves.");
        Slider("Shadow Strength", c.shadowStrength, 0.0f, 1.0f, "%.2f", "How much clouds darken the ground.");
        Color("Cloud Colour", c.color, "Tint of the clouds: grey storms, yellow sulfur, dark ash.");
        Slider("Quality (steps)", c.viewSteps, 16, 128, "Ray steps per pixel. 16 to 128. The biggest cloud cost.");
        ImGui::EndDisabled();
        ImGui::EndDisabled();
    }

    ImGui::EndDisabled();
}

} // namespace planets::render
