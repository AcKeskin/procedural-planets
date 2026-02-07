#pragma once

struct GLFWwindow;

namespace planets::app {

// Key codes matching GLFW
enum class Key
{
    W = 87,
    A = 65,
    S = 83,
    D = 68,
    Q = 81,
    E = 69,
    G = 71,
    H = 72,
    R = 82,
    Space = 32,
    LeftShift = 340,
    LeftControl = 341,
    Escape = 256
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

    float GetMouseX() const { return _mouseX; }
    float GetMouseY() const { return _mouseY; }
    float GetMouseDeltaX() const { return _mouseDeltaX; }
    float GetMouseDeltaY() const { return _mouseDeltaY; }

    void SetCursorEnabled(bool enabled);
    bool IsCursorEnabled() const { return _cursorEnabled; }

private:
    static constexpr int KeyStateSize = 512;

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
};

} // namespace planets::app
