#include "DebugPanel.h"
#include "../../core/math/Camera.h"
#include "../../core/math/CameraController.h"
#include "GuiLayout.h"
#include <imgui.h>

namespace planets::render
{

namespace
{
constexpr float MinFovDegrees = 20.0f;
constexpr float MaxFovDegrees = 100.0f;
constexpr float MaxOrbitSpeed = 30.0f;
} // namespace

// autoOrbit is unused: auto-rotate is read from and written to the camera, so the O key and
// this checkbox share one switch.
void DebugPanel::DrawContent(planets::core::Camera& camera, float& speedMultiplier, bool /*autoOrbit*/,
                             float& autoOrbitSpeed)
{
    PushLabelColumn();

    const auto& pos = camera.GetPosition();
    ImGui::Text("Camera Position: %.2f, %.2f, %.2f", pos.x, pos.y, pos.z);
    ImGui::Text("Yaw: %.2f  Pitch: %.2f", camera.GetYaw(), camera.GetPitch());

    const planets::core::CameraHud& hud = camera.Hud();
    const char* modeStr = camera.GetMode() == planets::core::CameraMode::FreeFly ? "Fly" : "Orbit";
    ImGui::Text("Mode: %s   Altitude: %.2f   Speed: %.2f", modeStr, hud.altitude, hud.speed);
    ImGui::SetItemTooltip("G switches Orbit / Fly. Speed is the fly speed at this altitude.");

    float fov = camera.GetFov();
    if (ImGui::SliderFloat("FOV", &fov, MinFovDegrees, MaxFovDegrees, "%.0f deg"))
        camera.SetFov(fov);
    ImGui::SetItemTooltip("Vertical field of view in degrees.");

    bool autoRotate = hud.autoRotate;
    if (ImGui::Checkbox("Auto-rotate (O)", &autoRotate))
        camera.Hud().autoRotate = autoRotate;
    ImGui::SetItemTooltip("Spin the camera around the planet (Orbit mode).");
    if (autoRotate)
        ImGui::SliderFloat("Orbit Speed", &autoOrbitSpeed, 1.0f, MaxOrbitSpeed, "%.1f deg/s");

    ImGui::Separator();
    ImGui::Text("Movement");
    ImGui::SliderFloat("Fly speed x", &speedMultiplier, planets::core::CameraController::MinSpeedMultiplier,
                       planets::core::CameraController::MaxSpeedMultiplier, "%.2f", ImGuiSliderFlags_Logarithmic);
    ImGui::SetItemTooltip("Multiplier on the altitude-based fly speed. The wheel changes it in Fly mode.");

    ImGui::PopItemWidth();
}

} // namespace planets::render
