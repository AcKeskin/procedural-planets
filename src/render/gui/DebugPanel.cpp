#include "DebugPanel.h"
#include "../../core/math/Camera.h"
#include <imgui.h>

namespace planets::render
{

void DebugPanel::Draw(
    const planets::core::Camera& camera, float& moveSpeed, bool autoOrbit, float& autoOrbitSpeed, bool& visible)
{
    if (!visible)
        return;

    ImGui::Begin("Debug", &visible);

    const auto& pos = camera.GetPosition();
    ImGui::Text("Camera Position: %.2f, %.2f, %.2f", pos.x, pos.y, pos.z);
    ImGui::Text("Yaw: %.2f  Pitch: %.2f", camera.GetYaw(), camera.GetPitch());

    const char* modeStr = camera.GetMode() == planets::core::CameraMode::FreeFly ? "Free-Fly" : "Orbit";
    ImGui::Text("Mode: %s", modeStr);

    ImGui::Text("Auto-Orbit: %s", autoOrbit ? "ON" : "OFF");
    if (autoOrbit)
        ImGui::SliderFloat("Orbit Speed", &autoOrbitSpeed, 1.0f, 30.0f, "%.1f deg/s");

    ImGui::Separator();
    ImGui::Text("Movement");
    ImGui::SliderFloat("Speed", &moveSpeed, 1.0f, 500.0f, "%.1f");

    ImGui::Separator();
    ImGui::Text("Controls:");
    ImGui::BulletText("WASD - Move");
    ImGui::BulletText("Mouse (RMB) - Look");
    ImGui::BulletText("Q/E - Up/Down");
    ImGui::BulletText("Shift - Boost");
    ImGui::BulletText("G - Toggle Auto-Orbit");
    ImGui::BulletText("H - Toggle Atmosphere");
    ImGui::BulletText("R - Shuffle Terrain");
    ImGui::BulletText("ESC - Exit");

    ImGui::End();
}

} // namespace planets::render
