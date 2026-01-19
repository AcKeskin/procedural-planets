#pragma once

struct GLFWwindow;

namespace planets::core {
class Camera;
class Planet;
}

namespace planets::render {

class Gui
{
public:
    Gui();
    ~Gui();

    Gui(const Gui&) = delete;
    Gui& operator=(const Gui&) = delete;

    bool Initialize(GLFWwindow* window);
    void Shutdown();

    void BeginFrame();
    void EndFrame();

    void DrawDebugPanel(const planets::core::Camera& camera);

    // Returns true if planet needs regeneration
    bool DrawPlanetPanel(planets::core::Planet& planet);

    // GPU compute panel with timing info
    void DrawGpuPanel(bool& useGpu, float cpuTimeMs, float gpuTimeMs, bool gpuAvailable);

private:
    bool _initialized;
};

} // namespace planets::render
