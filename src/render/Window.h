#pragma once

#include <string>

struct GLFWwindow;

namespace planets::render {

struct WindowConfig
{
    int width = 1280;
    int height = 720;
    std::string title = "Procedural Planets";
    int openglMajor = 4;
    int openglMinor = 5;
    bool vsync = true;
};

class Window
{
public:
    Window();
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    bool Initialize(const WindowConfig& config);
    void Shutdown();

    void PollEvents();
    void SwapBuffers();

    bool ShouldClose() const;
    void SetShouldClose(bool value);

    int GetWidth() const { return _width; }
    int GetHeight() const { return _height; }
    float GetAspectRatio() const;

    GLFWwindow* GetHandle() const { return _window; }

private:
    static void FramebufferSizeCallback(GLFWwindow* window, int width, int height);

    GLFWwindow* _window;
    int _width;
    int _height;
};

} // namespace planets::render
