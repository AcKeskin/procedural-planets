#include "Input.h"
#include <GLFW/glfw3.h>
#include <cstring>

namespace planets::app
{

Input::Input()
    : _window(nullptr)
    , _mouseX(0.0f)
    , _mouseY(0.0f)
    , _lastMouseX(0.0f)
    , _lastMouseY(0.0f)
    , _mouseDeltaX(0.0f)
    , _mouseDeltaY(0.0f)
    , _firstUpdate(true)
    , _cursorEnabled(true)
{
}

namespace
{
Input* g_scrollTarget = nullptr; // GLFW's scroll callback has no user pointer to spare
}

void Input::ScrollCallback(GLFWwindow*, double, double yoffset)
{
    if (g_scrollTarget)
        g_scrollTarget->_scrollAccum += static_cast<float>(yoffset);
}

void Input::Initialize(GLFWwindow* window)
{
    _window = window;
    _firstUpdate = true;

    // Installed before the GUI backend: ImGui chains to whatever callback was set first.
    g_scrollTarget = this;
    glfwSetScrollCallback(window, ScrollCallback);
}

void Input::Update()
{
    // Track key state transitions for edge detection
    std::memcpy(_prevKeyState, _keyState, sizeof(_keyState));
    for (int i = 0; i < KeyStateSize; ++i)
        _keyState[i] = (glfwGetKey(_window, i) == GLFW_PRESS);

    double xpos, ypos;
    glfwGetCursorPos(_window, &xpos, &ypos);

    _mouseX = static_cast<float>(xpos);
    _mouseY = static_cast<float>(ypos);

    if (_firstUpdate)
    {
        _lastMouseX = _mouseX;
        _lastMouseY = _mouseY;
        _firstUpdate = false;
    }

    _scrollDelta = _scrollAccum;
    _scrollAccum = 0.0f;

    const bool prevLeftDown = _leftDown;
    _leftDown = glfwGetMouseButton(_window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    _doubleClicked = false;
    if (_leftDown && !prevLeftDown)
    {
        const double now = glfwGetTime();
        const float dx = _mouseX - _lastClickX;
        const float dy = _mouseY - _lastClickY;
        if (now - _lastClickTime < DoubleClickSeconds && dx * dx + dy * dy < DoubleClickPixels * DoubleClickPixels)
        {
            _doubleClicked = true;
            _lastClickTime = -1.0; // a third click starts a new pair
        }
        else
        {
            _lastClickTime = now;
            _lastClickX = _mouseX;
            _lastClickY = _mouseY;
        }
    }

    _mouseDeltaX = _mouseX - _lastMouseX;
    _mouseDeltaY = _lastMouseY - _mouseY; // Inverted for camera

    _lastMouseX = _mouseX;
    _lastMouseY = _mouseY;
}

bool Input::IsKeyDown(Key key) const
{
    return glfwGetKey(_window, static_cast<int>(key)) == GLFW_PRESS;
}

bool Input::IsKeyPressed(Key key) const
{
    int k = static_cast<int>(key);
    if (k < 0 || k >= KeyStateSize)
        return false;
    return _keyState[k] && !_prevKeyState[k];
}

bool Input::IsMouseButtonDown(MouseButton button) const
{
    return glfwGetMouseButton(_window, static_cast<int>(button)) == GLFW_PRESS;
}

void Input::SetCursorEnabled(bool enabled)
{
    _cursorEnabled = enabled;
    glfwSetInputMode(_window, GLFW_CURSOR, enabled ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);

    if (!enabled)
    {
        _firstUpdate = true;
    }
}

} // namespace planets::app
