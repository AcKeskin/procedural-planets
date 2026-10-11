#include "PlanetWindow.h"
#include "GuiLayout.h"
#include "Widgets.h"
#include "../BodyRuntime.h"
#include "../settings/AtmosphereSettings.h"
#include "../settings/CinematicSettings.h"
#include "../settings/CloudSettings.h"
#include "../settings/OceanSettings.h"
#include "../settings/SceneSettings.h"
#include "../settings/SurfaceSettings.h"
#include "../settings/TerrainSettings.h"
#include <imgui.h>
#include <numeric>

namespace planets::render
{

namespace
{
using types::LookUnit;

const char* LockTip(LookUnit unit)
{
    return unit == LookUnit::Terrain ? "Reroll keeps these values. The land shape still follows the seed."
                                     : "Reroll and a type switch keep these values.";
}
} // namespace

size_t PlanetWindow::EditCount() const
{
    return std::accumulate(_edits.begin(), _edits.end(), size_t{0},
                           [](size_t n, const std::set<const void*>& e) { return n + e.size(); });
}

void PlanetWindow::ForgetEdits(uint32_t units)
{
    for (size_t i = 0; i < _edits.size(); ++i)
    {
        if (units & types::UnitBit(static_cast<LookUnit>(i)))
            _edits[i].clear();
    }
}

void PlanetWindow::Toolbar(LookUnit unit, const PlanetUi& ui, PlanetActions& actions)
{
    const uint32_t bit = types::UnitBit(unit);
    bool locked = (*ui.lockMask & bit) != 0;
    if (ImGui::Checkbox("Lock", &locked))
        *ui.lockMask = locked ? (*ui.lockMask | bit) : (*ui.lockMask & ~bit);
    ImGui::SetItemTooltip("%s", LockTip(unit));

    ImGui::SameLine();
    ImGui::BeginDisabled(locked || (ui.varietyUnits & bit) == 0);
    if (ImGui::Button("Randomize"))
        actions.randomizeUnit = static_cast<int>(unit);
    ImGui::SetItemTooltip("%s", (ui.varietyUnits & bit) == 0 ? "This type has no random values in this tab."
                                                             : "New values for this tab only. The seed stays.");
    ImGui::SameLine();
    if (ImGui::Button("Reset"))
        actions.resetUnit = static_cast<int>(unit);
    ImGui::SetItemTooltip("This tab back to the type's values at the current seed.");
    ImGui::EndDisabled();

    const size_t edited = _edits[static_cast<size_t>(unit)].size();
    ImGui::SameLine();
    if (edited > 0)
        ImGui::TextColored(ui::EditedColor, "%zu edited", edited);
    else
        ImGui::TextDisabled("0 edited");
    ImGui::Separator();

    ui::SetEditSet(&_edits[static_cast<size_t>(unit)]);
}

PlanetActions PlanetWindow::Draw(PlanetUi& ui)
{
    PlanetActions actions;
    if (!ImGui::Begin(WindowName, nullptr, ImGuiWindowFlags_NoCollapse))
    {
        ImGui::End();
        return actions;
    }

    ui::ResetTracking();
    if (ui.baseline)
    {
        const types::LookState& b = *ui.baseline;
        if (ui.body)
            ui::Track(ui.body->Config(), b.config);
        ui::Track(*ui.seaLevel, b.config.metadata.seaLevel);
        ui::Track(*ui.surface, b.surface);
        ui::Track(*ui.ocean, b.ocean);
        ui::Track(*ui.atmosphere, b.atmosphere);
        ui::Track(*ui.clouds, b.clouds);
        ui::Track(*ui.scene, b.scene);
    }

    if (ImGui::BeginTabBar("##PlanetTabs", ImGuiTabBarFlags_FittingPolicyResizeDown))
    {
        if (ImGui::BeginTabItem("Terrain"))
        {
            Toolbar(LookUnit::Terrain, ui, actions);
            PushLabelColumn();
            if (ui.body)
                actions.regenerate |= _terrain.Draw(*ui.body);
            ImGui::PopItemWidth();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Surface"))
        {
            Toolbar(LookUnit::Surface, ui, actions);
            PushLabelColumn();
            if (ui.body)
                actions.regenerate |= _surfaceTab.Draw(*ui.body, *ui.surface);
            ImGui::PopItemWidth();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Ocean"))
        {
            Toolbar(LookUnit::Ocean, ui, actions);
            PushLabelColumn();
            if (ui.body)
                actions.regenerate |= _ocean.Draw(*ui.body, *ui.ocean, *ui.seaLevel, *ui.surface);
            ImGui::PopItemWidth();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Sky"))
        {
            Toolbar(LookUnit::Sky, ui, actions);
            PushLabelColumn();
            _sky.Draw(*ui.atmosphere, *ui.clouds, ui.scene->lighting, !ui.body || ui.body->HasAtmosphere());
            ImGui::PopItemWidth();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Light"))
        {
            Toolbar(LookUnit::Light, ui, actions);
            PushLabelColumn();
            _light.Draw(*ui.scene, *ui.surface);
            ImGui::PopItemWidth();
            ImGui::EndTabItem();
        }
        // Camera and System are not look tabs: no amber, no edit records.
        ui::ResetTracking();
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
