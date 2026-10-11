#include "HelpOverlay.h"
#include "Keybinds.h"
#include <nlohmann/json.hpp>
#include <imgui.h>
#include <fstream>

namespace planets::app
{

namespace
{
constexpr const char* StatePath = "gui_state.json";
}

bool IsHelpSeen(const nlohmann::json& state)
{
    if (!state.is_object())
        return false;
    const auto it = state.find("helpSeen");
    return it != state.end() && it->is_boolean() && it->get<bool>();
}

void HelpOverlay::LoadFirstLaunchState()
{
    std::ifstream in(StatePath);
    nlohmann::json state = nlohmann::json::parse(in, nullptr, false);
    if (IsHelpSeen(state))
        return;

    _open = true;
    if (!state.is_object())
        state = nlohmann::json::object();
    state["helpSeen"] = true;
    std::ofstream out(StatePath);
    out << state.dump(2) << '\n';
}

void HelpOverlay::Draw()
{
    if (!_open)
        return;

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    constexpr ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings |
                                       ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoMove |
                                       ImGuiWindowFlags_AlwaysAutoResize;
    if (!ImGui::Begin("Controls", &_open, flags))
    {
        ImGui::End();
        return;
    }

    for (KeyGroup group : {KeyGroup::Camera, KeyGroup::Planet, KeyGroup::View})
    {
        ImGui::SeparatorText(Keybinds::GroupName(group));
        if (!ImGui::BeginTable(Keybinds::GroupName(group), 2, ImGuiTableFlags_SizingFixedFit))
            continue;
        ImGui::TableSetupColumn("key", ImGuiTableColumnFlags_WidthFixed, ImGui::GetFontSize() * 8.0f);
        ImGui::TableSetupColumn("action", ImGuiTableColumnFlags_WidthFixed, ImGui::GetFontSize() * 24.0f);
        for (const KeyBind& row : Keybinds::Table)
        {
            if (row.group != group)
                continue;
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::TextUnformatted(row.label);
            ImGui::TableSetColumnIndex(1);
            ImGui::TextUnformatted(row.text);
        }
        ImGui::EndTable();
    }

    ImGui::Spacing();
    ImGui::TextDisabled("F1 or Esc closes this.");
    ImGui::End();
}

} // namespace planets::app
