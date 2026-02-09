#include "CinematicPanel.h"
#include "../settings/CinematicSettings.h"
#include "../../app/Keybinds.h"
#include <imgui.h>
#include <algorithm>

namespace planets::render
{

namespace
{
constexpr float PlayButtonHeight = 40.0f;
constexpr const char* DurationModeItems = "Rotations\0Seconds\0Loop\0";
} // namespace

void CinematicPanel::Draw(CinematicSettings& settings, float planetRadius, bool& playRequested, bool& visible)
{
    if (!visible)
        return;

    ImGui::Begin("Cinematic", &visible);

    if (ImGui::CollapsingHeader("Turntable", ImGuiTreeNodeFlags_DefaultOpen))
        DrawTurntableContent(settings.turntable, planetRadius);

    if (ImGui::CollapsingHeader("Duration", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Text("Playback Mode");
        ImGui::Separator();

        int modeIndex = static_cast<int>(settings.turntable.durationMode);
        if (ImGui::Combo("Mode", &modeIndex, DurationModeItems))
        {
            settings.turntable.durationMode = static_cast<CinematicDurationMode>(modeIndex);
        }

        if (settings.turntable.durationMode != CinematicDurationMode::Loop)
        {
            if (settings.turntable.durationMode == CinematicDurationMode::Rotations)
            {
                ImGui::SliderFloat("Rotations",
                                   &settings.turntable.durationValue,
                                   CinematicLimits::MinRotations,
                                   CinematicLimits::MaxRotations,
                                   "%.1f");
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Number of full rotations before stopping");
            }
            else
            {
                ImGui::SliderFloat("Seconds",
                                   &settings.turntable.durationValue,
                                   CinematicLimits::MinDurationSeconds,
                                   CinematicLimits::MaxDurationSeconds,
                                   "%.1f s");
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Duration in seconds before stopping");
            }
        }
    }

    ImGui::Separator();
    if (ImGui::Button("Play", ImVec2(-1, PlayButtonHeight)))
    {
        playRequested = true;
    }

    if (ImGui::CollapsingHeader("Hotkeys"))
    {
        using namespace planets::app;
        ImGui::BulletText("%s", Keybinds::CinematicToggle);
        ImGui::BulletText("%s", Keybinds::Screenshot);
        ImGui::BulletText("%s", Keybinds::CinematicStop);
        ImGui::BulletText("%s", Keybinds::GuiToggle);
    }

    ImGui::End();
}

void CinematicPanel::DrawTurntableContent(TurntableSettings& settings, float planetRadius)
{
    ImGui::Text("Rotation");
    ImGui::Separator();

    ImGui::SliderFloat("Orbit Speed",
                       &settings.orbitSpeed,
                       CinematicLimits::MinOrbitSpeed,
                       CinematicLimits::MaxOrbitSpeed,
                       "%.1f deg/s");
    ImGui::Checkbox("Speed Easing", &settings.speedEasing);
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Smooth acceleration and deceleration at start and end");

    ImGui::Separator();
    ImGui::Text("Pitch");

    ImGui::Checkbox("Pitch Oscillation", &settings.pitchOscillation);
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Vertical camera movement during orbit");

    if (settings.pitchOscillation)
    {
        ImGui::SliderFloat("Amplitude",
                           &settings.pitchAmplitude,
                           CinematicLimits::MinPitchAmplitude,
                           CinematicLimits::MaxPitchAmplitude,
                           "%.1f deg");
        ImGui::SliderFloat("Period",
                           &settings.pitchPeriod,
                           CinematicLimits::MinPitchPeriod,
                           CinematicLimits::MaxPitchPeriod,
                           "%.1f s");
    }

    ImGui::Separator();
    ImGui::Text("Zoom");

    ImGui::InputFloat("Start Zoom",
                      &settings.zoomStartMultiplier,
                      CinematicLimits::ZoomMultiplierStep,
                      CinematicLimits::ZoomMultiplierFastStep,
                      "%.2fx");
    settings.zoomStartMultiplier =
        (std::max)(CinematicLimits::MinZoomMultiplier,
                   (std::min)(settings.zoomStartMultiplier, CinematicLimits::MaxZoomMultiplier));
    ImGui::SameLine();
    ImGui::TextDisabled("(%.0f units)", settings.zoomStartMultiplier * planetRadius);

    ImGui::InputFloat("End Zoom",
                      &settings.zoomEndMultiplier,
                      CinematicLimits::ZoomMultiplierStep,
                      CinematicLimits::ZoomMultiplierFastStep,
                      "%.2fx");
    settings.zoomEndMultiplier = (std::max)(CinematicLimits::MinZoomMultiplier,
                                            (std::min)(settings.zoomEndMultiplier, CinematicLimits::MaxZoomMultiplier));
    ImGui::SameLine();
    ImGui::TextDisabled("(%.0f units)", settings.zoomEndMultiplier * planetRadius);

    ImGui::Checkbox("Zoom Easing", &settings.zoomEasing);
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Smooth distance interpolation");
}

} // namespace planets::render
