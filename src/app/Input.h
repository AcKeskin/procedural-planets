#pragma once

struct GLFWwindow;

namespace planets::app
{

// Key codes matching GLFW
enum class Key
{
    None = 0,
    Num1 = 49, // Num1..Num9 are consecutive
    W = 87,
    A = 65,
    S = 83,
    D = 68,
    Q = 81,
    E = 69,
    F = 70,
    G = 71,
    H = 72,
    O = 79,
    R = 82,
    Space = 32,
    LeftShift = 340,
    LeftControl = 341,
    Escape = 256,
    Tab = 258,
    Home = 268,
    F1 = 290,
    F5 = 294,
    F12 = 301
};

enum class MouseButton
{
    Left = 0,
    Right = 1,
    Middle = 2
};

class Input
{
public:
    Input();

    void Initialize(GLFWwindow* window);
    void Update();

    bool IsKeyDown(Key key) const;
    bool IsKeyPressed(Key key) const;
    bool IsMouseButtonDown(MouseButton button) const;
    bool WasDoubleClicked() const { return _doubleClicked; } // left button, this frame

    // Wheel notches since the last Update (positive = away from the user).
    float GetScrollDelta() const { return _scrollDelta; }

    float GetMouseX() const { return _mouseX; }
    float GetMouseY() const { return _mouseY; }
    float GetMouseDeltaX() const { return _mouseDeltaX; }
    float GetMouseDeltaY() const { return _mouseDeltaY; }

    void SetCursorEnabled(bool enabled);
    bool IsCursorEnabled() const { return _cursorEnabled; }

private:
    static constexpr int KeyStateSize = 512;
    static constexpr double DoubleClickSeconds = 0.3;
    static constexpr float DoubleClickPixels = 6.0f;

    static void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);

    GLFWwindow* _window;
    bool _keyState[KeyStateSize] = {};
    bool _prevKeyState[KeyStateSize] = {};
    float _mouseX;
    float _mouseY;
    float _lastMouseX;
    float _lastMouseY;
    float _mouseDeltaX;
    float _mouseDeltaY;
    bool _firstUpdate;
    bool _cursorEnabled;

    float _scrollAccum = 0.0f; // filled by the GLFW callback between frames
    float _scrollDelta = 0.0f;
    bool _leftDown = false;
    bool _doubleClicked = false;
    double _lastClickTime = -1.0;
    float _lastClickX = 0.0f;
    float _lastClickY = 0.0f;
};

} // namespace planets::app
