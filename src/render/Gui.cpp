#include "Gui.h"
#include "../core/math/Camera.h"
#include "../core/generation/Planet.h"
#include "../core/generation/NoiseLayer.h"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <GLFW/glfw3.h>
#include <string>

namespace planets::render {

Gui::Gui()
    : _initialized(false)
{
}

Gui::~Gui()
{
    Shutdown();
}

bool Gui::Initialize(GLFWwindow* window)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 450");

    _initialized = true;
    return true;
}

void Gui::Shutdown()
{
    if (_initialized)
    {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        _initialized = false;
    }
}

void Gui::BeginFrame()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void Gui::EndFrame()
{
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void Gui::DrawDebugPanel(const planets::core::Camera& camera)
{
    ImGui::Begin("Debug");

    const auto& pos = camera.GetPosition();
    ImGui::Text("Camera Position: %.2f, %.2f, %.2f", pos.x, pos.y, pos.z);
    ImGui::Text("Yaw: %.2f  Pitch: %.2f", camera.GetYaw(), camera.GetPitch());

    const char* modeStr = camera.GetMode() == planets::core::CameraMode::FreeFly ? "Free-Fly" : "Orbit";
    ImGui::Text("Mode: %s", modeStr);

    ImGui::Separator();
    ImGui::Text("Controls:");
    ImGui::BulletText("WASD - Move");
    ImGui::BulletText("Mouse (RMB) - Look");
    ImGui::BulletText("Q/E - Up/Down");
    ImGui::BulletText("ESC - Exit");

    ImGui::End();
}

bool Gui::DrawPlanetPanel(planets::core::Planet& planet)
{
    bool needsRegeneration = false;

    ImGui::Begin("Planet");

    auto& settings = planet.GetSettings();

    ImGui::Text("Planet Settings");
    ImGui::Separator();

    if (ImGui::SliderFloat("Radius", &settings.radius, 0.5f, 5.0f))
    {
        needsRegeneration = true;
    }

    if (ImGui::SliderFloat("Sea Level", &settings.seaLevel, 0.0f, 1.0f))
    {
        // Sea level doesn't require mesh regeneration, just shader uniform update
    }

    if (ImGui::SliderFloat("Height Scale", &settings.heightScale, 0.0f, 0.2f))
    {
        needsRegeneration = true;
    }

    if (ImGui::SliderInt("Subdivisions", &settings.subdivisions, 1, 6))
    {
        needsRegeneration = true;
    }

    int seed = static_cast<int>(settings.seed);
    if (ImGui::InputInt("Seed", &seed))
    {
        settings.seed = static_cast<uint32_t>(seed);
        planet.Reseed(settings.seed);
        needsRegeneration = true;
    }

    ImGui::Separator();
    ImGui::Text("Noise Layers: %zu", planet.GetNoiseLayerCount());

    for (size_t i = 0; i < planet.GetNoiseLayerCount(); ++i)
    {
        auto& layer = planet.GetNoiseLayer(i);
        auto& layerSettings = layer.GetSettings();

        ImGui::PushID(static_cast<int>(i));

        if (ImGui::CollapsingHeader(("Layer " + std::to_string(i)).c_str()))
        {
            if (ImGui::Checkbox("Enabled", &layerSettings.enabled))
            {
                layer.Configure(layerSettings);
                needsRegeneration = true;
            }

            if (ImGui::SliderFloat("Scale", &layerSettings.scale, 0.1f, 10.0f))
            {
                layer.Configure(layerSettings);
                needsRegeneration = true;
            }

            if (ImGui::SliderFloat("Strength", &layerSettings.strength, 0.0f, 2.0f))
            {
                layer.Configure(layerSettings);
                needsRegeneration = true;
            }

            if (ImGui::SliderInt("Octaves", &layerSettings.octaves, 1, 8))
            {
                layer.Configure(layerSettings);
                needsRegeneration = true;
            }

            if (ImGui::SliderFloat("Persistence", &layerSettings.persistence, 0.1f, 1.0f))
            {
                layer.Configure(layerSettings);
                needsRegeneration = true;
            }

            if (ImGui::SliderFloat("Lacunarity", &layerSettings.lacunarity, 1.0f, 4.0f))
            {
                layer.Configure(layerSettings);
                needsRegeneration = true;
            }
        }

        ImGui::PopID();
    }

    if (ImGui::Button("Regenerate"))
    {
        needsRegeneration = true;
    }

    ImGui::End();

    return needsRegeneration;
}

void Gui::DrawGpuPanel(bool& useGpu, float cpuTimeMs, float gpuTimeMs, bool gpuAvailable)
{
    ImGui::Begin("GPU Compute");

    if (!gpuAvailable)
    {
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "GPU compute not available");
        ImGui::Text("Using CPU fallback");
    }
    else
    {
        ImGui::Checkbox("Use GPU", &useGpu);

        ImGui::Separator();
        ImGui::Text("Generation Times:");
        ImGui::BulletText("CPU: %.2f ms", cpuTimeMs);
        ImGui::BulletText("GPU: %.2f ms", gpuTimeMs);

        if (cpuTimeMs > 0.0f && gpuTimeMs > 0.0f)
        {
            float speedup = cpuTimeMs / gpuTimeMs;
            ImGui::Text("Speedup: %.1fx", speedup);
        }
    }

    ImGui::End();
}

} // namespace planets::render
