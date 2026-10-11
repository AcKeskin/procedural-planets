#include "Toast.h"
#include <imgui.h>
#include <algorithm>
#include <cstdio>

namespace planets::app
{

namespace
{
constexpr float FadeSeconds = 0.5f;
constexpr float EdgeMargin = 16.0f;

constexpr ImGuiWindowFlags OverlayFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs |
                                          ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove |
                                          ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
                                          ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_AlwaysAutoResize;
} // namespace

void Toasts::Show(std::string text)
{
    _items.push_back({std::move(text), LifetimeSeconds});
    if (_items.size() > MaxVisible)
        _items.erase(_items.begin());
}

void Toasts::Draw()
{
    const float dt = ImGui::GetIO().DeltaTime;
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    float y = viewport->WorkPos.y + viewport->WorkSize.y - EdgeMargin;
    const float x = viewport->WorkPos.x + viewport->WorkSize.x - EdgeMargin;

    int index = 0;
    for (Item& item : _items)
    {
        item.remaining -= dt;
        const float alpha = std::clamp(item.remaining / FadeSeconds, 0.0f, 1.0f);
        const ImVec2 size = ImGui::CalcTextSize(item.text.c_str());
        const ImVec2 pad = ImGui::GetStyle().WindowPadding;

        ImGui::SetNextWindowPos(ImVec2(x, y), ImGuiCond_Always, ImVec2(1.0f, 1.0f));
        ImGui::SetNextWindowBgAlpha(0.85f * alpha);
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);
        char id[24];
        std::snprintf(id, sizeof(id), "##toast%d", index++);
        if (ImGui::Begin(id, nullptr, OverlayFlags))
            ImGui::TextUnformatted(item.text.c_str());
        ImGui::End();
        ImGui::PopStyleVar();

        y -= size.y + pad.y * 2.0f + 6.0f;
    }

    _items.erase(std::remove_if(_items.begin(), _items.end(), [](const Item& i) { return i.remaining <= 0.0f; }),
                 _items.end());
}

void Toasts::DrawHoldHint(const char* text, float fraction)
{
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(viewport->GetCenter().x, viewport->WorkPos.y + viewport->WorkSize.y * 0.8f),
                            ImGuiCond_Always,
                            ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowBgAlpha(0.85f);
    if (ImGui::Begin("##holdhint", nullptr, OverlayFlags))
    {
        ImGui::TextUnformatted(text);
        ImGui::ProgressBar(std::clamp(fraction, 0.0f, 1.0f), ImVec2(ImGui::CalcTextSize(text).x, 4.0f), "");
    }
    ImGui::End();
}

} // namespace planets::app
