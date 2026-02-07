#include "AtmospherePanel.h"
#include "../settings/AtmosphereSettings.h"
#include <imgui.h>
#include <glm/glm.hpp>

namespace planets::render {

void AtmospherePanel::Draw(effects::AtmosphereSettings& settings, bool& visible)
{
    if (!visible) return;

    ImGui::Begin("Atmosphere", &visible);

    ImGui::Checkbox("Enable Atmosphere", &settings.enabled);

    if (settings.enabled)
    {
        ImGui::Separator();
        ImGui::Text("Geometry");
        ImGui::SliderFloat("Atmosphere Scale", &settings.atmosphereScale, 0.02f, 0.3f, "%.3f");
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Height as fraction of planet radius");

        ImGui::Separator();
        ImGui::Text("Rayleigh Scattering");

        ImGui::SliderFloat("Red Wavelength", &settings.wavelengths.x, 550.0f, 780.0f, "%.0f nm");
        ImGui::SliderFloat("Green Wavelength", &settings.wavelengths.y, 450.0f, 600.0f, "%.0f nm");
        ImGui::SliderFloat("Blue Wavelength", &settings.wavelengths.z, 380.0f, 500.0f, "%.0f nm");
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Shorter wavelengths scatter more (blue sky)");

        ImGui::SliderFloat("Scattering Strength", &settings.scatteringStrength, 1.0f, 30.0f);

        ImGui::Separator();
        ImGui::Text("Density");
        ImGui::SliderFloat("Density Falloff", &settings.densityFalloff, 1.0f, 20.0f);
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("How quickly density decreases with altitude");

        ImGui::Separator();
        ImGui::Text("Quality");
        ImGui::SliderInt("In-Scatter Samples", &settings.numInScatteringPoints, 3, 20);
        ImGui::SliderInt("Optical Depth Samples", &settings.numOpticalDepthPoints, 3, 20);
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("More samples = better quality, slower");

        ImGui::Separator();
        ImGui::SliderFloat("Intensity", &settings.intensity, 0.1f, 50.0f);

        ImGui::Separator();
        ImGui::Text("Dithering");
        ImGui::SliderFloat("Dither Strength", &settings.ditherStrength, 0.0f, 2.0f);
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Blue noise dithering to reduce banding");
        ImGui::SliderFloat("Dither Scale", &settings.ditherScale, 1.0f, 10.0f);
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Scale of the noise pattern");

        ImGui::Separator();
        if (ImGui::Button("Earth Preset"))
        {
            settings.wavelengths = glm::vec3(700.0f, 530.0f, 460.0f);
            settings.scatteringStrength = 3.0f;
            settings.densityFalloff = 10.0f;
            settings.atmosphereScale = 0.2f;
            settings.intensity = 2.0f;
            settings.ditherStrength = 0.8f;
            settings.ditherScale = 4.0f;
        }
        ImGui::SameLine();
        if (ImGui::Button("Mars Preset"))
        {
            settings.wavelengths = glm::vec3(600.0f, 550.0f, 500.0f);
            settings.scatteringStrength = 1.5f;
            settings.densityFalloff = 5.0f;
            settings.atmosphereScale = 0.04f;
            settings.intensity = 8.0f;
            settings.ditherStrength = 0.8f;
            settings.ditherScale = 4.0f;
        }
    }

    ImGui::End();
}

} // namespace planets::render
