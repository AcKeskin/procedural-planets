#pragma once

struct GLFWwindow;

namespace planets::core {
class Camera;
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

private:
    bool _initialized;
};

} // namespace planets::render
