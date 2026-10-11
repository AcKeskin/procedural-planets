#include "CameraTab.h"
#include "Widgets.h"
#include "../../core/math/Camera.h"
#include "../../core/math/CameraController.h"
#include "../settings/CinematicSettings.h"
#include <imgui.h>

namespace planets::render
{

namespace
{
constexpr float MinFovDegrees = 20.0f;
constexpr float MaxFovDegrees = 100.0f;
constexpr float PlayButtonHeight = 32.0f;
// "Stop After" lists Never first; the enum keeps its order for scripts.
constexpr const char* StopAfterItems = "Never\0Turns\0Seconds\0";
constexpr CinematicDurationMode StopAfterModes[] = {CinematicDurationMode::Loop, CinematicDurationMode::Rotations,
                                                    CinematicDurationMode::Seconds};
} // namespace

void CameraTab::Draw(core::Camera& camera, float& flySpeed, CinematicSettings& cinematic, bool& play)
{
    using namespace ui;
    namespace limits = CinematicLimits;
    auto& t = cinematic.turntable;

    if (ImGui::CollapsingHeader("View", ImGuiTreeNodeFlags_DefaultOpen))
    {
        const core::CameraHud& hud = camera.Hud();
        const bool fly = camera.GetMode() == core::CameraMode::FreeFly;
        ImGui::Text("%s   altitude %.2f   speed %.2f", fly ? "Fly" : "Orbit", hud.altitude, hud.speed);
        ImGui::SetItemTooltip("G switches Orbit / Fly. Speed is the fly speed at this altitude.");

        float fov = camera.GetFov();
        if (Slider("FOV", fov, MinFovDegrees, MaxFovDegrees, "%.0f deg", "Vertical field of view. 20 to 100 degrees."))
            camera.SetFov(fov);
        Slider("Fly Speed", flySpeed, core::CameraController::MinSpeedMultiplier,
               core::CameraController::MaxSpeedMultiplier, "x%.2f",
               "Multiplier on the altitude-based fly speed. The wheel sets it in Fly.", ImGuiSliderFlags_Logarithmic);

        if (ImGui::TreeNode("Position"))
        {
            const auto& pos = camera.GetPosition();
            ImGui::Text("%.2f, %.2f, %.2f", pos.x, pos.y, pos.z);
            ImGui::Text("yaw %.1f  pitch %.1f", camera.GetYaw(), camera.GetPitch());
            ImGui::TreePop();
        }
    }

    if (ImGui::CollapsingHeader("Orbit Motion", ImGuiTreeNodeFlags_DefaultOpen))
    {
        bool autoRotate = camera.Hud().autoRotate;
        if (Check("Auto-rotate (O)", autoRotate, "Spin the camera around the planet. Key: O."))
            camera.Hud().autoRotate = autoRotate;
        // One speed for auto-rotate and the turntable.
        Slider("Orbit Speed", t.orbitSpeed, limits::MinOrbitSpeed, limits::MaxOrbitSpeed, "%.1f deg/s",
               "Auto-rotate and turntable speed. 1 to 60 degrees per second.");
    }

    if (ImGui::CollapsingHeader("Turntable (F5)", ImGuiTreeNodeFlags_DefaultOpen))
    {
        Check("Speed Easing", t.speedEasing, "Ease in and out at the start and end.");
        Slider("Start Yaw", t.startYaw, limits::MinStartYaw, limits::MaxStartYaw, "%.0f deg",
               "Angle around the planet the orbit starts from.");
        Slider("Start Pitch", t.startPitch, limits::MinStartPitch, limits::MaxStartPitch, "%.0f deg",
               "Height angle the orbit starts from. 0 = equator.");

        Check("Pitch Swing", t.pitchOscillation, "Move the camera up and down during the orbit.");
        if (t.pitchOscillation)
        {
            Slider("Swing Size", t.pitchAmplitude, limits::MinPitchAmplitude, limits::MaxPitchAmplitude, "%.1f deg",
                   "How far up and down the camera moves. 1 to 30 degrees.");
            Slider("Swing Period", t.pitchPeriod, limits::MinPitchPeriod, limits::MaxPitchPeriod, "%.1f s",
                   "Seconds for one up-and-down swing.");
        }

        Slider("Start Distance", t.zoomStartMultiplier, limits::MinZoomMultiplier, limits::MaxZoomMultiplier,
               "%.2f radii", "Camera distance at the start, in planet radii.", ImGuiSliderFlags_Logarithmic);
        Slider("End Distance", t.zoomEndMultiplier, limits::MinZoomMultiplier, limits::MaxZoomMultiplier,
               "%.2f radii", "Camera distance at the end, in planet radii.", ImGuiSliderFlags_Logarithmic);
        Check("Zoom Easing", t.zoomEasing, "Ease the distance change.");

        int item = 0;
        for (int i = 0; i < 3; ++i)
        {
            if (StopAfterModes[i] == t.durationMode)
                item = i;
        }
        if (ImGui::Combo("Stop After", &item, StopAfterItems))
            t.durationMode = StopAfterModes[item];
        ImGui::SetItemTooltip("Never = keep turning until F5 or Esc. Or stop after some turns or seconds.");
        if (t.durationMode == CinematicDurationMode::Rotations)
            Slider("Turns", t.durationValue, limits::MinRotations, limits::MaxRotations, "%.1f",
                   "Full turns before it stops. 1 to 10.");
        else if (t.durationMode == CinematicDurationMode::Seconds)
            Slider("Seconds", t.durationValue, limits::MinDurationSeconds, limits::MaxDurationSeconds, "%.1f s",
                   "Seconds before it stops. 1 to 120.");

        if (ImGui::Button("Play (F5)", ImVec2(-1.0f, PlayButtonHeight)))
            play = true;
        ImGui::SetItemTooltip("Start the turntable. The window hides; Esc stops it.");
    }
}

} // namespace planets::render
