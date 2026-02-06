#pragma once

struct GLFWwindow;

namespace planets::render {

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

    struct PanelVisibility
    {
        bool scene = true;
        bool terrain = true;
        bool surface = true;
        bool atmosphere = true;
        bool debug = true;
    };

    PanelVisibility& GetVisibility() { return _visibility; }
    const PanelVisibility& GetVisibility() const { return _visibility; }

private:
    void SetupDockspace();

    bool _initialized;
    bool _resetLayout;
    PanelVisibility _visibility;
};

} // namespace planets::render
