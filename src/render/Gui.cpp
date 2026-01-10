#include "Gui.h"
#include "../core/math/Camera.h"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <GLFW/glfw3.h>

namespace planets::render {

Gui::Gui()
    : _initialized(false)
{
}

Gui::~Gui()
{
    Shutdown();
}

bool Gui::Initialize(GLFWwindow* window)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 450");

    _initialized = true;
    return true;
}

void Gui::Shutdown()
{
    if (_initialized)
    {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        _initialized = false;
    }
}

void Gui::BeginFrame()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void Gui::EndFrame()
{
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void Gui::DrawDebugPanel(const planets::core::Camera& camera)
{
    ImGui::Begin("Debug");

    const auto& pos = camera.GetPosition();
    ImGui::Text("Camera Position: %.2f, %.2f, %.2f", pos.x, pos.y, pos.z);
    ImGui::Text("Yaw: %.2f  Pitch: %.2f", camera.GetYaw(), camera.GetPitch());

    const char* modeStr = camera.GetMode() == planets::core::CameraMode::FreeFly ? "Free-Fly" : "Orbit";
    ImGui::Text("Mode: %s", modeStr);

    ImGui::Separator();
    ImGui::Text("Controls:");
    ImGui::BulletText("WASD - Move");
    ImGui::BulletText("Mouse (RMB) - Look");
    ImGui::BulletText("Q/E - Up/Down");
    ImGui::BulletText("ESC - Exit");

    ImGui::End();
}

} // namespace planets::render
