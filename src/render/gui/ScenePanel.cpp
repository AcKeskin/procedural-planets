#include "ScenePanel.h"
#include "../settings/SceneSettings.h"
#include "GuiLayout.h"
#include <imgui.h>
#include <cmath>

namespace planets::render
{

void ScenePanel::DrawContent(SceneSettings& settings, bool atmosphereOwnsHaze)
{
    PushLabelColumn();

    if (ImGui::CollapsingHeader("Space & Sun", ImGuiTreeNodeFlags_DefaultOpen))
        DrawSpaceContent(settings);

    if (ImGui::CollapsingHeader("Lighting", ImGuiTreeNodeFlags_DefaultOpen))
        DrawLightingContent(settings.lighting, atmosphereOwnsHaze);

    ImGui::PopItemWidth();
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

    ImGui::SliderFloat("Sun Size", &settings.sunSize, 0.002f, 0.05f, "%.4f rad", ImGuiSliderFlags_Logarithmic);
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Angular radius of the sun disc. The real sun is 0.0047 rad");

    ImGui::ColorEdit3("Sun Color", &settings.sunColor.x);

    ImGui::Separator();
    ImGui::Text("Stars");

    ImGui::SliderFloat("Star Density", &settings.starDensity, 0.0f, 3.0f);

    ImGui::Separator();
    ImGui::Text("Camera");

    ImGui::SliderFloat("Exposure", &settings.exposure, 0.25f, 4.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Brightness of the whole image before highlights roll off to white");
    ImGui::SliderFloat("Bloom", &settings.bloomStrength, 0.0f, 3.0f, "%.2f");
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Glow around very bright light: the sun and sun glints on water");
}

void ScenePanel::DrawLightingContent(LightingSettings& settings, bool atmosphereOwnsHaze)
{
    ImGui::Text("Sun Lighting");
    ImGui::Separator();

    ImGui::SliderFloat("Sun Intensity", &settings.sunIntensity, 0.6f, 1.8f, "%.2f");
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Overall brightness of sunlight on terrain");

    ImGui::SliderFloat("Ambient Light", &settings.ambientLight, 0.0f, 0.5f, "%.3f");
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Base illumination in shadows");

    ImGui::Separator();
    ImGui::Text("Terminator");

    ImGui::SliderFloat("Terminator Softness", &settings.terminatorSoftness, 0.0f, 0.3f, "%.3f");
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Width of the soft day/night twilight band (0 = hard edge). PBR shader only");

    ImGui::SliderFloat("Night Lights", &settings.nightLightStrength, 0.0f, 2.0f, "%.2f");
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Emissive city lights on the habitable night side (0 = off). PBR shader only");

    ImGui::Separator();
    ImGui::Text("Specular");

    ImGui::SliderFloat("Specular Strength", &settings.specularStrength, 0.0f, 0.5f, "%.2f");
    ImGui::SetItemTooltip("Generic shader only; the PBR shader (Earth) derives specular from roughness");
    ImGui::SliderFloat("Specular Power", &settings.specularPower, 16.0f, 64.0f, "%.0f");
    ImGui::SetItemTooltip("Generic shader only; the PBR shader (Earth) derives specular from roughness");

    ImGui::Separator();
    ImGui::Text("Atmosphere & Detail");

    ImGui::SliderFloat("Detail Fade", &settings.detailFadeStart, 1.0f, 5.0f, "%.1f");
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Distance (planet radii) where surface detail begins fading");

    // The atmosphere pass owns distance fade while it is on; the renderer zeroes this haze then.
    ImGui::BeginDisabled(atmosphereOwnsHaze);
    ImGui::SliderFloat("Haze Strength", &settings.hazeStrength, 0.0f, 0.4f, "%.2f");
    ImGui::SetItemTooltip(atmosphereOwnsHaze ? "Off while the atmosphere is enabled (it handles distance fade)"
                                             : "Atmospheric perspective: desaturation at distance");
    ImGui::ColorEdit3("Haze Color", &settings.hazeColor.x);
    ImGui::EndDisabled();

    ImGui::Separator();
    if (ImGui::Button("Reset"))
    {
        settings = LightingSettings();
    }
}

} // namespace planets::render
