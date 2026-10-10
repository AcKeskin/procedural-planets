#include "CloudsPanel.h"
#include "../settings/CloudSettings.h"
#include "GuiLayout.h"
#include <imgui.h>

namespace planets::render
{

void CloudsPanel::Draw(effects::CloudSettings& settings, bool atmosphereOn, bool& visible)
{
    if (!visible)
        return;

    ImGui::Begin("Clouds", &visible);
    PushLabelColumn();

    ImGui::BeginDisabled(!atmosphereOn);
    ImGui::Checkbox("Enable Clouds", &settings.enabled);
    ImGui::SetItemTooltip(atmosphereOn ? "Show the cloud layer" : "Clouds are part of the atmosphere: enable the atmosphere to show them");
    if (settings.enabled)
    {
        ImGui::Separator();
        ImGui::SliderFloat("Coverage", &settings.coverage, 0.0f, 1.0f, "%.2f");
        ImGui::SetItemTooltip("0 = clear sky, 1 = overcast");
        ImGui::SliderFloat("Density", &settings.density, 20.0f, 2000.0f, "%.0f", ImGuiSliderFlags_Logarithmic);
        ImGui::SetItemTooltip("How thick the clouds are: low = wispy and see-through, high = solid");
        ImGui::SliderFloat("Cloud Size", &settings.scale, 0.25f, 4.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
        ImGui::SetItemTooltip("Size of individual clouds (higher = smaller clouds)");
        ImGui::SliderFloat("Base Altitude", &settings.altitude, 0.0f, 0.1f, "%.3f");
        ImGui::SetItemTooltip("Cloud base above sea level, as a fraction of planet radius (kept above the mountains)");
        ImGui::SliderFloat("Thickness", &settings.thickness, 0.005f, 0.08f, "%.3f");
        ImGui::SetItemTooltip("Depth of the cloud layer, as a fraction of planet radius");
        ImGui::SliderFloat("Drift Speed", &settings.speed, 0.0f, 1.0f, "%.2f");
        ImGui::SetItemTooltip("How fast the weather moves around the planet");
        ImGui::SliderFloat("Shadow Strength", &settings.shadowStrength, 0.0f, 1.0f, "%.2f");
        ImGui::SetItemTooltip("How much clouds darken the ground below");
        ImGui::SliderInt("Quality", &settings.viewSteps, 16, 128);
        ImGui::SetItemTooltip("March steps per pixel: higher = smoother clouds, slower");
    }
    ImGui::EndDisabled();

    ImGui::PopItemWidth();
    ImGui::End();
}

} // namespace planets::render
