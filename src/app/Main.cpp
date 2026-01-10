#include "Input.h"
#include "Window.h"
#include "Gui.h"
#include "Renderer.h"
#include "math/Camera.h"
#include <GLFW/glfw3.h>

int main()
{
    planets::render::Window window;
    planets::render::WindowConfig windowConfig;
    windowConfig.title = "Procedural Planets";

    if (!window.Initialize(windowConfig))
    {
        return -1;
    }

    planets::app::Input input;
    input.Initialize(window.GetHandle());

    planets::render::Renderer renderer;
    renderer.Initialize();

    planets::render::Gui gui;
    gui.Initialize(window.GetHandle());

    planets::core::Camera camera;
    camera.SetPosition(glm::vec3(0.0f, 0.0f, 10.0f));

    float lastTime = static_cast<float>(glfwGetTime());
    const float moveSpeed = 5.0f;
    const float lookSpeed = 0.1f;

    while (!window.ShouldClose())
    {
        float currentTime = static_cast<float>(glfwGetTime());
        float deltaTime = currentTime - lastTime;
        lastTime = currentTime;

        window.PollEvents();
        input.Update();

        // Exit on ESC
        if (input.IsKeyDown(planets::app::Key::Escape))
        {
            window.SetShouldClose(true);
        }

        // Toggle cursor capture with right mouse button
        if (input.IsMouseButtonDown(planets::app::MouseButton::Right))
        {
            if (input.IsCursorEnabled())
            {
                input.SetCursorEnabled(false);
            }

            // Camera look
            camera.Rotate(input.GetMouseDeltaX() * lookSpeed, input.GetMouseDeltaY() * lookSpeed);
        }
        else
        {
            if (!input.IsCursorEnabled())
            {
                input.SetCursorEnabled(true);
            }
        }

        // Camera movement
        float speed = moveSpeed * deltaTime;
        if (input.IsKeyDown(planets::app::Key::W))
            camera.MoveForward(speed);
        if (input.IsKeyDown(planets::app::Key::S))
            camera.MoveForward(-speed);
        if (input.IsKeyDown(planets::app::Key::A))
            camera.MoveRight(-speed);
        if (input.IsKeyDown(planets::app::Key::D))
            camera.MoveRight(speed);
        if (input.IsKeyDown(planets::app::Key::E))
            camera.MoveUp(speed);
        if (input.IsKeyDown(planets::app::Key::Q))
            camera.MoveUp(-speed);

        // Render
        renderer.BeginFrame();

        gui.BeginFrame();
        gui.DrawDebugPanel(camera);
        gui.EndFrame();

        renderer.EndFrame();
        window.SwapBuffers();
    }

    gui.Shutdown();
    renderer.Shutdown();
    window.Shutdown();

    return 0;
}
