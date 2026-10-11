#include "GuiManager.h"
#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <GLFW/glfw3.h>

namespace planets::render
{

GuiManager::GuiManager()
    : _initialized(false)
    , _resetLayout(false)
{
}

GuiManager::~GuiManager()
{
    Shutdown();
}

bool GuiManager::Initialize(GLFWwindow* window)
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

void GuiManager::Shutdown()
{
    if (_initialized)
    {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        _initialized = false;
    }
}

void GuiManager::BeginFrame()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void GuiManager::EndFrame()
{
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void GuiManager::DrawDockspace()
{
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                             ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                             ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus |
                             ImGuiWindowFlags_NoBackground;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("##Dockspace", nullptr, flags);
    ImGui::PopStyleVar(3);

    // Renamed when the default layout changes, so saved imgui.ini layouts pick up the new one.
    ImGuiID dockspaceId = ImGui::GetID("MainDockspaceV3");

    bool needsBuild = (ImGui::DockBuilderGetNode(dockspaceId) == nullptr) || _resetLayout;
    _resetLayout = false;

    if (needsBuild)
    {
        ImGui::DockBuilderRemoveNode(dockspaceId);
        ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodeSize(dockspaceId, viewport->WorkSize);

        // One Planet window on the left, about 440 px at 1920 wide; the viewport gets the rest.
        ImGuiID dockMain = dockspaceId;
        const float leftShare = viewport->WorkSize.x > 0.0f ? 440.0f / viewport->WorkSize.x : 0.25f;
        ImGuiID dockLeft = ImGui::DockBuilderSplitNode(dockMain, ImGuiDir_Left, leftShare, nullptr, &dockMain);
        ImGui::DockBuilderDockWindow("Planet", dockLeft);

        ImGui::DockBuilderFinish(dockspaceId);
    }

    ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);
    ImGui::End();
}

} // namespace planets::render
