#pragma once

struct GLFWwindow;

namespace planets::render
{

class GuiManager
{
public:
    GuiManager();
    ~GuiManager();

    GuiManager(const GuiManager&) = delete;
    GuiManager& operator=(const GuiManager&) = delete;

    bool Initialize(GLFWwindow* window);
    void Shutdown();

    void BeginFrame();
    void EndFrame();

    // The full-window dock host; call after the main menu bar so it sits below it.
    void DrawDockspace();

    // Rebuild the default dock layout on the next frame (View > Reset Layout).
    void RequestResetLayout() { _resetLayout = true; }

private:
    bool _initialized;
    bool _resetLayout;
};

} // namespace planets::render
