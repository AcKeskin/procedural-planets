#include "Window.h"
#include <GL/gl3w.h>
#include <GLFW/glfw3.h>
#include <iostream>

namespace planets::render
{

Window::Window()
    : _window(nullptr)
    , _width(0)
    , _height(0)
{
}

Window::~Window()
{
    Shutdown();
}

bool Window::Initialize(const WindowConfig& config)
{
    if (!glfwInit())
    {
        std::cerr << "[Window] Failed to initialize GLFW" << std::endl;
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, config.openglMajor);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, config.openglMinor);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    _window = glfwCreateWindow(config.width, config.height, config.title.c_str(), nullptr, nullptr);
    if (!_window)
    {
        std::cerr << "[Window] Failed to create window" << std::endl;
        glfwTerminate();
        return false;
    }

    _width = config.width;
    _height = config.height;

    glfwMakeContextCurrent(_window);

    if (gl3wInit() != 0)
    {
        std::cerr << "[Window] Failed to initialize gl3w" << std::endl;
        glfwDestroyWindow(_window);
        glfwTerminate();
        return false;
    }

    std::cout << "[Window] OpenGL " << glGetString(GL_VERSION) << std::endl;
    std::cout << "[Window] Created " << _width << "x" << _height << " window" << std::endl;

    if (config.vsync)
    {
        glfwSwapInterval(1);
    }

    glfwSetWindowUserPointer(_window, this);
    glfwSetFramebufferSizeCallback(_window, FramebufferSizeCallback);

    glViewport(0, 0, _width, _height);

    return true;
}

void Window::Shutdown()
{
    if (_window)
    {
        glfwDestroyWindow(_window);
        _window = nullptr;
    }
    glfwTerminate();
}

void Window::PollEvents()
{
    glfwPollEvents();
}

void Window::SwapBuffers()
{
    glfwSwapBuffers(_window);
}

bool Window::ShouldClose() const
{
    return glfwWindowShouldClose(_window);
}

void Window::SetShouldClose(bool value)
{
    glfwSetWindowShouldClose(_window, value ? GLFW_TRUE : GLFW_FALSE);
}

float Window::GetAspectRatio() const
{
    if (_height == 0)
        return 1.0f;
    return static_cast<float>(_width) / static_cast<float>(_height);
}

void Window::FramebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    Window* self = static_cast<Window*>(glfwGetWindowUserPointer(window));
    if (self)
    {
        self->_width = width;
        self->_height = height;
        glViewport(0, 0, width, height);
    }
}

} // namespace planets::render
