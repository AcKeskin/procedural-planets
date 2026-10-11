#pragma once

#include <glm/glm.hpp>
#include <imgui.h>

namespace planets::render::ui
{

// Every control in the Planet window goes through these: a tooltip is required, and slow
// controls report a change only when the edit is committed (mouse released), so a drag does
// not rebuild the planet every frame.

inline bool Committed()
{
    return ImGui::IsItemDeactivatedAfterEdit();
}

// Live slider: returns true on any change.
inline bool Slider(const char* label, float& v, float lo, float hi, const char* fmt, const char* tip,
                   ImGuiSliderFlags flags = 0)
{
    const bool changed = ImGui::SliderFloat(label, &v, lo, hi, fmt, flags);
    ImGui::SetItemTooltip("%s", tip);
    return changed;
}

inline bool Slider(const char* label, int& v, int lo, int hi, const char* tip)
{
    const bool changed = ImGui::SliderInt(label, &v, lo, hi);
    ImGui::SetItemTooltip("%s", tip);
    return changed;
}

// Slider whose change regenerates the terrain: true only when the edit is committed.
inline bool SliderRegen(const char* label, float& v, float lo, float hi, const char* fmt, const char* tip,
                        ImGuiSliderFlags flags = 0)
{
    ImGui::SliderFloat(label, &v, lo, hi, fmt, flags);
    ImGui::SetItemTooltip("%s", tip);
    return Committed();
}

inline bool SliderRegen(const char* label, int& v, int lo, int hi, const char* tip)
{
    ImGui::SliderInt(label, &v, lo, hi);
    ImGui::SetItemTooltip("%s", tip);
    return Committed();
}

inline bool Range(const char* label, glm::vec2& v, float lo, float hi, const char* tip)
{
    const bool changed = ImGui::SliderFloat2(label, &v.x, lo, hi, "%.2f");
    ImGui::SetItemTooltip("%s", tip);
    return changed;
}

inline bool Color(const char* label, glm::vec3& c, const char* tip)
{
    const bool changed = ImGui::ColorEdit3(label, &c.x, ImGuiColorEditFlags_NoInputs);
    ImGui::SetItemTooltip("%s", tip);
    return changed;
}

inline bool Check(const char* label, bool& v, const char* tip)
{
    const bool changed = ImGui::Checkbox(label, &v);
    ImGui::SetItemTooltip("%s", tip);
    return changed;
}

} // namespace planets::render::ui
