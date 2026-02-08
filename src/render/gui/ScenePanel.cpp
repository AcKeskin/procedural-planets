#include "ScenePanel.h"
#include "../settings/SceneSettings.h"
#include <imgui.h>
#include <cmath>

namespace planets::render {

void ScenePanel::Draw(SceneSettings& settings, bool& visible)
{
    if (!visible) return;

    ImGui::Begin("Scene", &visible);

    if (ImGui::CollapsingHeader("Space & Sun", ImGuiTreeNodeFlags_DefaultOpen))
        DrawSpaceContent(settings);

    if (ImGui::CollapsingHeader("Lighting", ImGuiTreeNodeFlags_DefaultOpen))
        DrawLightingContent(settings.lighting);

    ImGui::End();
}

void ScenePanel::DrawSpaceContent(SceneSettings& settings)
{
    ImGui::Text("Sun Direction");
    ImGui::Separator();

    float lightLength = glm::length(settings.lightDir);
    glm::vec3 normalizedDir = settings.lightDir / lightLength;

    float azimuth = atan2(normalizedDir.x, normalizedDir.z);
    float elevation = asin(normalizedDir.y);

    bool changed = false;
    changed |= ImGui::SliderAngle("Azimuth", &azimuth, -180.0f, 180.0f);
    changed |= ImGui::SliderAngle("Elevation", &elevation, -90.0f, 90.0f);

    if (changed)
    {
        settings.lightDir.x = cos(elevation) * sin(azimuth);
        settings.lightDir.y = sin(elevation);
        settings.lightDir.z = cos(elevation) * cos(azimuth);
        settings.lightDir = glm::normalize(settings.lightDir);
    }

    ImGui::Separator();
    ImGui::Text("Sun Appearance");

    ImGui::SliderFloat("Sun Size", &settings.sunSize, 0.01f, 0.1f, "%.3f rad");
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Angular size in radians");

    ImGui::ColorEdit3("Sun Color", &settings.sunColor.x);

    ImGui::Separator();
    ImGui::Text("Stars");

    ImGui::SliderFloat("Star Density", &settings.starDensity, 0.0f, 3.0f);
}

void ScenePanel::DrawLightingContent(LightingSettings& settings)
{
    ImGui::Text("Sun Lighting");
    ImGui::Separator();

    ImGui::SliderFloat("Sun Intensity", &settings.sunIntensity, 0.6f, 1.8f, "%.2f");
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Overall brightness of sunlight on terrain");

    ImGui::SliderFloat("Ambient Light", &settings.ambientLight, 0.05f, 0.25f, "%.3f");
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Base illumination in shadows");

    ImGui::Separator();
    ImGui::Text("Specular");

    ImGui::SliderFloat("Specular Strength", &settings.specularStrength, 0.0f, 0.5f, "%.2f");
    ImGui::SliderFloat("Specular Power", &settings.specularPower, 16.0f, 64.0f, "%.0f");

    ImGui::Separator();
    if (ImGui::Button("Reset"))
    {
        settings = LightingSettings();
    }
}

} // namespace planets::render
