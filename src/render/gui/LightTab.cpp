#include "LightTab.h"
#include "Widgets.h"
#include "../settings/SceneSettings.h"
#include "../settings/SurfaceSettings.h"
#include <cmath>
#include <imgui.h>

namespace planets::render
{

void LightTab::Draw(SceneSettings& scene, SurfaceSettings& surface)
{
    using namespace ui;
    auto& l = scene.lighting;

    if (ImGui::CollapsingHeader("Sun", ImGuiTreeNodeFlags_DefaultOpen))
    {
        const glm::vec3 dir = glm::normalize(scene.lightDir);
        float azimuth = std::atan2(dir.x, dir.z);
        float elevation = std::asin(dir.y);
        // Both angles edit lightDir, so they share one edited state and one right-click menu.
        const bool edited = BeginValue(scene.lightDir);
        bool turned = ImGui::SliderAngle("Sun Azimuth", &azimuth, -180.0f, 180.0f);
        ImGui::SetItemTooltip("Sun direction around the planet, in degrees.");
        turned |= ImGui::SliderAngle("Sun Elevation", &elevation, -90.0f, 90.0f);
        if (turned)
            scene.lightDir = glm::normalize(glm::vec3(std::cos(elevation) * std::sin(azimuth), std::sin(elevation),
                                                      std::cos(elevation) * std::cos(azimuth)));
        EndValue(scene.lightDir, edited, "Sun height in degrees. 0 = on the equator plane.");
        Slider("Sun Intensity", l.sunIntensity, 0.6f, 1.8f, "%.2f", "Brightness of sunlight on the ground.");
        Color("Sun Colour", scene.sunColor, "Colour of the star's light.");
        Slider("Sun Size", scene.sunSize, 0.002f, 0.05f, "%.4f rad", "Angular radius of the sun disc. Real sun: 0.0047.",
               ImGuiSliderFlags_Logarithmic);
    }

    if (ImGui::CollapsingHeader("Shadows and Night", ImGuiTreeNodeFlags_DefaultOpen))
    {
        Slider("Ambient", l.ambientLight, 0.0f, 0.5f, "%.3f", "Light in full shadow. 0 = black shadows.");
        if (ImGui::TreeNode("Shadow Fill"))
        {
            auto& a = surface.skyAmbient;
            Slider("Sky Ambient", a.strength, 0.0f, 1.0f, "%.2f",
                   "Shadows take the sky colour from above, ground colour from below. 0 = grey.");
            Color("Sky Fill", a.skyColor, "Tint the sky adds to shadows. Keep it near white in brightness.");
            Color("Ground Bounce", a.groundColor, "Tint bounced up from the ground onto steep faces.");
            Slider("From Sky", a.fromSky, 0.0f, 1.0f, "%.2f", "1 = the sky fill takes its colour from the atmosphere.");
            ImGui::TreePop();
        }
        Slider("Twilight Width", l.terminatorSoftness, 0.0f, 0.3f, "%.3f", "Width of the day/night band. 0 = hard edge.");
        Slider("Night Lights", l.nightLightStrength, 0.0f, 2.0f, "%.2f",
               "Brightness of city lights on the night side. 0 = off.");
    }

    if (ImGui::CollapsingHeader("Camera Exposure", ImGuiTreeNodeFlags_DefaultOpen))
    {
        Slider("Exposure", scene.exposure, 0.25f, 4.0f, "%.2f", "Brightness of the whole image before highlights roll off.",
               ImGuiSliderFlags_Logarithmic);
        Slider("Bloom", scene.bloomStrength, 0.0f, 3.0f, "%.2f", "Glow around very bright light: the sun and glints.");
        Slider("Star Density", scene.starDensity, 0.0f, 3.0f, "%.2f", "How many background stars. 0 = none.");
    }
}

} // namespace planets::render
