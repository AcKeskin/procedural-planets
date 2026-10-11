#pragma once

#include <glm/glm.hpp>
#include <imgui.h>
#include <cstddef>
#include <cstring>
#include <set>
#include <typeinfo>
#include <vector>

namespace planets::render::ui
{

// Every control in the Planet window goes through these: a tooltip is required, and slow
// controls report a change only when the edit is committed (mouse released), so a drag does
// not rebuild the planet every frame.
//
// A value that differs from its rolled default shows in amber; right-click gives Reset value,
// Copy and Paste. The default is found by the value's offset inside a tracked struct.

namespace detail
{
struct Region
{
    const unsigned char* live;
    const unsigned char* base;
    size_t size;
};

struct Tracking
{
    std::vector<Region> regions;
    std::set<const void*>* edits = nullptr; // the drawn tab's edited values, keyed by default address
    bool menuChanged = false;
    std::vector<unsigned char> clip;
    const std::type_info* clipType = nullptr;
};

inline Tracking& State()
{
    static Tracking t;
    return t;
}

inline const void* Base(const void* value, size_t size)
{
    const auto* p = static_cast<const unsigned char*>(value);
    for (const Region& r : State().regions)
    {
        if (p >= r.live && p + size <= r.live + r.size)
            return r.base + (p - r.live);
    }
    return nullptr;
}

template <class T> bool IsEdited(const T& v)
{
    const void* base = Base(&v, sizeof(T));
    return base && std::memcmp(&v, base, sizeof(T)) != 0;
}
} // namespace detail

inline const ImVec4 EditedColor{1.0f, 0.72f, 0.25f, 1.0f};

// Values inside `live` are measured against the same offset in `base` (the same struct type).
template <class T> void Track(const T& live, const T& base)
{
    detail::State().regions.push_back(
        {reinterpret_cast<const unsigned char*>(&live), reinterpret_cast<const unsigned char*>(&base), sizeof(T)});
}

// Stops tracking; `edits` (may be null) collects the edited values the next controls draw.
inline void ResetTracking(std::set<const void*>* edits = nullptr)
{
    detail::State().regions.clear();
    detail::State().edits = edits;
}

inline void SetEditSet(std::set<const void*>* edits)
{
    detail::State().edits = edits;
}

// Before a control: amber text when the value is edited. Pass the result to EndValue.
template <class T> bool BeginValue(const T& v)
{
    detail::State().menuChanged = false;
    const bool edited = detail::IsEdited(v);
    if (edited)
        ImGui::PushStyleColor(ImGuiCol_Text, EditedColor);
    return edited;
}

// After a control: tooltip, right-click menu and the edit record. True when the menu changed v.
template <class T> bool EndValue(T& v, bool pushed, const char* tip)
{
    auto& state = detail::State();
    if (pushed)
        ImGui::PopStyleColor();
    ImGui::SetItemTooltip("%s", tip);

    bool changed = false;
    if (ImGui::BeginPopupContextItem())
    {
        const void* base = detail::Base(&v, sizeof(T));
        if (ImGui::MenuItem("Reset value", nullptr, false, base && detail::IsEdited(v)))
        {
            std::memcpy(&v, base, sizeof(T));
            changed = true;
        }
        if (ImGui::MenuItem("Copy"))
        {
            const auto* bytes = reinterpret_cast<const unsigned char*>(&v);
            state.clip.assign(bytes, bytes + sizeof(T));
            state.clipType = &typeid(T);
        }
        if (ImGui::MenuItem("Paste", nullptr, false, state.clipType && *state.clipType == typeid(T)))
        {
            std::memcpy(&v, state.clip.data(), sizeof(T));
            changed = true;
        }
        ImGui::EndPopup();
    }
    state.menuChanged = changed;

    if (state.edits)
    {
        if (const void* base = detail::Base(&v, sizeof(T)))
        {
            if (std::memcmp(&v, base, sizeof(T)) != 0)
                state.edits->insert(base);
            else
                state.edits->erase(base);
        }
    }
    return changed;
}

// True when the last control's edit is done: mouse released, or a Reset/Paste from its menu.
inline bool Committed()
{
    return ImGui::IsItemDeactivatedAfterEdit() || detail::State().menuChanged;
}

// Live slider: returns true on any change.
inline bool
Slider(const char* label, float& v, float lo, float hi, const char* fmt, const char* tip, ImGuiSliderFlags flags = 0)
{
    const bool edited = BeginValue(v);
    bool changed = ImGui::SliderFloat(label, &v, lo, hi, fmt, flags);
    changed |= EndValue(v, edited, tip);
    return changed;
}

inline bool Slider(const char* label, int& v, int lo, int hi, const char* tip)
{
    const bool edited = BeginValue(v);
    bool changed = ImGui::SliderInt(label, &v, lo, hi);
    changed |= EndValue(v, edited, tip);
    return changed;
}

// Slider whose change regenerates the terrain: true only when the edit is committed.
inline bool SliderRegen(
    const char* label, float& v, float lo, float hi, const char* fmt, const char* tip, ImGuiSliderFlags flags = 0)
{
    const bool edited = BeginValue(v);
    ImGui::SliderFloat(label, &v, lo, hi, fmt, flags);
    EndValue(v, edited, tip);
    return Committed();
}

inline bool SliderRegen(const char* label, int& v, int lo, int hi, const char* tip)
{
    const bool edited = BeginValue(v);
    ImGui::SliderInt(label, &v, lo, hi);
    EndValue(v, edited, tip);
    return Committed();
}

inline bool Range(const char* label, glm::vec2& v, float lo, float hi, const char* tip)
{
    const bool edited = BeginValue(v);
    bool changed = ImGui::SliderFloat2(label, &v.x, lo, hi, "%.2f");
    changed |= EndValue(v, edited, tip);
    return changed;
}

inline bool Color(const char* label, glm::vec3& c, const char* tip)
{
    // NoOptions: right-click opens the Reset/Copy/Paste menu, not the colour options.
    const bool edited = BeginValue(c);
    bool changed = ImGui::ColorEdit3(label, &c.x, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoOptions);
    changed |= EndValue(c, edited, tip);
    return changed;
}

inline bool Check(const char* label, bool& v, const char* tip)
{
    const bool edited = BeginValue(v);
    bool changed = ImGui::Checkbox(label, &v);
    changed |= EndValue(v, edited, tip);
    return changed;
}

} // namespace planets::render::ui
