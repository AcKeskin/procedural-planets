#pragma once

#include <string>
#include <vector>

namespace planets::app
{

// Short messages in the bottom-right corner (screenshot saved, new seed, bookmark).
class Toasts
{
public:
    static constexpr float LifetimeSeconds = 2.5f;
    static constexpr size_t MaxVisible = 4;

    void Show(std::string text);
    void Draw(); // call inside an ImGui frame

    // Centered hint with a fill bar, for holds like "Hold Esc to quit". fraction is 0..1.
    static void DrawHoldHint(const char* text, float fraction);

private:
    struct Item
    {
        std::string text;
        float remaining;
    };

    std::vector<Item> _items;
};

} // namespace planets::app
