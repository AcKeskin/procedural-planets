#include "CloudsPanel.h"
#include "../settings/CloudSettings.h"
#include "GuiLayout.h"
#include <imgui.h>

namespace planets::render
{

void CloudsPanel::Draw(effects::CloudSettings& settings, bool& visible)
{
    if (!visible)
        return;

    ImGui::Begin("Clouds", &visible);
    PushLabelColumn();

    ImGui::Checkbox("Enable Clouds", &settings.enabled);
    if (settings.enabled)
    {
        ImGui::Separator();
        ImGui::SliderFloat("Coverage", &settings.coverage, 0.0f, 1.0f, "%.2f");
        ImGui::SetItemTooltip("0 = clear sky, 1 = overcast");
        ImGui::SliderFloat("Density", &settings.density, 0.0f, 1.0f, "%.2f");
        ImGui::SetItemTooltip("Opacity of the covered regions");
        ImGui::SliderFloat("Scale", &settings.scale, 0.5f, 10.0f, "%.1f");
        ImGui::SetItemTooltip("Size of the cloud pattern (higher = smaller clouds)");
        ImGui::SliderFloat("Altitude", &settings.altitude, 0.0f, 0.1f, "%.3f");
        ImGui::SetItemTooltip("Shell height above the surface, as a fraction of planet radius");
        ImGui::SliderFloat("Drift Speed", &settings.speed, 0.0f, 1.0f, "%.2f");
        ImGui::SliderFloat("Shadow Strength", &settings.shadowStrength, 0.0f, 1.0f, "%.2f");
        ImGui::SetItemTooltip("How much clouds darken the ground below");
        ImGui::ColorEdit3("Color", &settings.color.x);
    }

    ImGui::PopItemWidth();
    ImGui::End();
}

} // namespace planets::render
