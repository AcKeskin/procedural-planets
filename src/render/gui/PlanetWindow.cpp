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
            PushLabelColumn();
            if (ui.body)
                actions.regenerate |= _terrain.Draw(*ui.body);
            ImGui::PopItemWidth();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Surface"))
        {
            PushLabelColumn();
            if (ui.body)
                actions.regenerate |= _surfaceTab.Draw(*ui.body, *ui.surface);
            ImGui::PopItemWidth();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Ocean"))
        {
            PushLabelColumn();
            if (ui.body)
                actions.regenerate |= _ocean.Draw(*ui.body, *ui.ocean, *ui.seaLevel, *ui.surface);
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
            actions.regenerate |= _system.Draw(*ui.lod, *ui.stats, ui.frameMs);
            ImGui::PopItemWidth();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    ImGui::End();
    return actions;
}

} // namespace planets::render
