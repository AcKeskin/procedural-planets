#include "PlanetWindow.h"
#include "GuiLayout.h"
#include "../settings/AtmosphereSettings.h"
#include "../settings/CinematicSettings.h"
#include "../settings/CloudSettings.h"
#include "../settings/OceanSettings.h"
#include "../settings/SceneSettings.h"
#include "../settings/SurfaceSettings.h"
#include "../settings/TerrainSettings.h"
#include <imgui.h>

namespace planets::render
{

PlanetActions PlanetWindow::Draw(PlanetUi& ui)
{
    PlanetActions actions;
    if (!ImGui::Begin(WindowName, nullptr, ImGuiWindowFlags_NoCollapse))
    {
        ImGui::End();
        return actions;
    }
    if (ImGui::BeginTabBar("##PlanetTabs", ImGuiTabBarFlags_FittingPolicyResizeDown))
    {
        if (ImGui::BeginTabItem("Terrain"))
        {
            actions.regenerate |= _terrain.DrawContent(ui.body, *ui.gen, *ui.lod, *ui.stats, actions.randomize);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Surface"))
        {
            actions.regenerate |= _surface.DrawContent(ui.body, *ui.surface, *ui.ocean, *ui.seaLevel);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Ocean"))
        {
            PushLabelColumn();
            _surface.DrawOceanContent(*ui.ocean, *ui.seaLevel);
            ImGui::PopItemWidth();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Sky"))
        {
            _atmosphere.DrawContent(*ui.atmosphere);
            ImGui::SeparatorText("Clouds");
            _clouds.DrawContent(*ui.clouds, ui.atmosphere->enabled);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Light"))
        {
            _scene.DrawContent(*ui.scene, ui.atmosphere->enabled);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Camera"))
        {
            _debug.DrawContent(*ui.camera, *ui.moveSpeed, ui.autoOrbit, *ui.autoOrbitSpeed);
            ImGui::SeparatorText("Turntable");
            _cinematic.DrawContent(*ui.cinematic, ui.planetRadius, actions.playCinematic);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("System"))
        {
            PushLabelColumn();
            ImGui::Text("Patches: %d active, %d visible", ui.stats->patchCount, ui.stats->visiblePatchCount);
            ImGui::Text("GPU: terrain %.2f / ocean %.2f / effects %.2f ms", ui.stats->gpuTerrainMs,
                        ui.stats->gpuOceanMs, ui.stats->gpuEffectsMs);
            ImGui::PopItemWidth();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    ImGui::End();
    return actions;
}

} // namespace planets::render
