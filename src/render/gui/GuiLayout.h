#pragma once

#include <imgui.h>

namespace planets::render
{

// Width kept free for the label to the right of each widget, in font sizes. ImGui's default
// item width leaves too little room in a side dock, so labels were cut off ("Contine...").
inline constexpr float kLabelColumnEm = 11.0f;

// Call right after ImGui::Begin; pair with ImGui::PopItemWidth() before ImGui::End.
inline void PushLabelColumn()
{
    ImGui::PushItemWidth(-ImGui::GetFontSize() * kLabelColumnEm);
}

} // namespace planets::render
