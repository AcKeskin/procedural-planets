#include "AtmospherePanel.h"
#include "../settings/AtmosphereSettings.h"
#include "GuiLayout.h"
#include <imgui.h>
#include <glm/glm.hpp>

namespace planets::render
{

void AtmospherePanel::Draw(effects::AtmosphereSettings& settings, bool& visible)
{
    if (!visible)
        return;

    ImGui::Begin("Atmosphere", &visible);
    PushLabelColumn();

    ImGui::Checkbox("Enable Atmosphere", &settings.enabled);

    if (settings.enabled)
    {
        ImGui::Separator();
        ImGui::Text("Geometry");
        ImGui::SliderFloat("Atmosphere Scale", &settings.atmosphereScale, 0.05f, 0.50f, "%.3f");
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Height as fraction of planet radius");

        ImGui::Separator();
        ImGui::Text("Rayleigh Scattering");

        // Rayleigh: scattering by air molecules; it gives the sky its colour.
        const char* wavelengthTip = "Shorter wavelengths scatter more (blue sky)";
        ImGui::SliderFloat("Red Wavelength", &settings.wavelengths.x, 620.0f, 740.0f, "%.0f nm");
        ImGui::SetItemTooltip("%s", wavelengthTip);
        ImGui::SliderFloat("Green Wavelength", &settings.wavelengths.y, 480.0f, 560.0f, "%.0f nm");
        ImGui::SetItemTooltip("%s", wavelengthTip);
        ImGui::SliderFloat("Blue Wavelength", &settings.wavelengths.z, 400.0f, 480.0f, "%.0f nm");
        ImGui::SetItemTooltip("%s", wavelengthTip);

        ImGui::SliderFloat("Scattering Strength", &settings.scatteringStrength, 5.0f, 40.0f);
        ImGui::SetItemTooltip("Amount of air scattering: higher = thicker, brighter sky");

        ImGui::Separator();
        ImGui::Text("Mie Scattering");
        ImGui::SetItemTooltip("Scattering by dust and droplets: haze and the glow around the sun");
        ImGui::SliderFloat("Mie Coefficient", &settings.mieCoefficient, 0.0f, 2.0f, "%.2f");
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Amount of haze: dims the far surface and brightens the glow around the sun");
        ImGui::SliderFloat("Mie Density Falloff", &settings.mieDensityFalloff, 1.0f, 20.0f);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("How quickly Mie density decreases with altitude");

        ImGui::Separator();
        ImGui::Text("Density");
        ImGui::SliderFloat("Density Falloff", &settings.densityFalloff, 1.0f, 10.0f);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("How quickly density decreases with altitude");

        ImGui::Separator();
        ImGui::Text("Quality");
        ImGui::SliderInt("In-Scatter Samples", &settings.numInScatteringPoints, 3, 20);
        ImGui::SliderInt("Optical Depth Samples", &settings.numOpticalDepthPoints, 3, 20);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("More samples = better quality, slower");

        ImGui::Separator();
        ImGui::SliderFloat("Intensity", &settings.intensity, 0.5f, 3.0f);
        ImGui::SetItemTooltip("Overall brightness of the scattered light");

        ImGui::Separator();
        ImGui::Text("Dithering");
        ImGui::SliderFloat("Dither Strength", &settings.ditherStrength, 0.0f, 2.0f);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Blue noise dithering to reduce banding");
        ImGui::SliderFloat("Dither Scale", &settings.ditherScale, 1.0f, 10.0f);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Scale of the noise pattern");

        ImGui::Separator();
        if (ImGui::Button("Earth Preset"))
        {
            settings = effects::AtmosphereSettings{}; // the defaults are the Earth look
            settings.ditherStrength = 0.8f;
            settings.ditherScale = 4.0f;
        }
        ImGui::SameLine();
        if (ImGui::Button("Mars Preset"))
        {
            settings.wavelengths = glm::vec3(600.0f, 550.0f, 500.0f);
            settings.scatteringStrength = 8.0f;
            settings.densityFalloff = 3.0f;
            settings.atmosphereScale = 0.08f;
            settings.mieCoefficient = 1.2f;
            settings.mieDensityFalloff = 2.5f;
            settings.intensity = 1.2f;
            settings.ditherStrength = 0.8f;
            settings.ditherScale = 4.0f;
        }
    }

    ImGui::PopItemWidth();
    ImGui::End();
}

} // namespace planets::render
