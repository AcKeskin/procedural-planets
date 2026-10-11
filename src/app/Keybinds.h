#pragma once

#include "Input.h"
#include <array>

namespace planets::app
{

enum class Action
{
    Fly,
    Vertical,
    Speed,
    Zoom,
    OrbitDrag,
    Look,
    ToggleMode,
    AutoRotate,
    FramePlanet,
    ResetView,
    FlyToPoint,
    RecallBookmark,
    SaveBookmark,
    Reroll,
    Atmosphere,
    HidePanel,
    Help,
    Turntable,
    Screenshot,
    Escape
};

enum class Modifier
{
    None,
    Ctrl
};

enum class KeyGroup
{
    Camera,
    Planet,
    View
};

// One row per control. ProcessInput reads key + modifier; the F1 help reads label, text and
// group. Rows with Key::None are mouse or held-key controls.
struct KeyBind
{
    Action action;
    Key key;
    Modifier modifier;
    const char* label;
    const char* text;
    KeyGroup group;
};

namespace Keybinds
{

// Held keys. Kept here so the whole key map lives in one file.
inline constexpr Key MoveForward = Key::W;
inline constexpr Key MoveBack = Key::S;
inline constexpr Key MoveLeft = Key::A;
inline constexpr Key MoveRight = Key::D;
inline constexpr Key MoveDown = Key::Q;
inline constexpr Key MoveUp = Key::E;
inline constexpr Key Faster = Key::LeftShift;
inline constexpr Key Slower = Key::LeftControl;
inline constexpr Key RightCtrl = static_cast<Key>(345); // GLFW_KEY_RIGHT_CONTROL

inline constexpr float FasterFactor = 4.0f;
inline constexpr float SlowerFactor = 0.25f;
inline constexpr float QuitHoldSeconds = 1.0f;

inline constexpr std::array<KeyBind, 20> Table{{
    {Action::Fly,
     Key::None,
     Modifier::None,
     "W A S D",
     "Fly forward, left, back, right (from Orbit too)",
     KeyGroup::Camera},
    {Action::Vertical, Key::None, Modifier::None, "Q / E", "Fly down / up", KeyGroup::Camera},
    {Action::Speed, Key::None, Modifier::None, "Shift / Ctrl", "Faster x4 / slower x0.25", KeyGroup::Camera},
    {Action::Zoom, Key::None, Modifier::None, "Wheel", "Zoom (Orbit) or fly speed (Fly)", KeyGroup::Camera},
    {Action::OrbitDrag, Key::None, Modifier::None, "LMB drag", "Orbit the planet", KeyGroup::Camera},
    {Action::Look, Key::None, Modifier::None, "RMB drag", "Look around (switches to Fly)", KeyGroup::Camera},
    {Action::ToggleMode, Key::G, Modifier::None, "G", "Switch Orbit / Fly", KeyGroup::Camera},
    {Action::AutoRotate, Key::O, Modifier::None, "O", "Auto-rotate", KeyGroup::Camera},
    {Action::FramePlanet, Key::F, Modifier::None, "F", "Frame the planet", KeyGroup::Camera},
    {Action::ResetView, Key::Home, Modifier::None, "Home", "Reset the view", KeyGroup::Camera},
    {Action::FlyToPoint, Key::None, Modifier::None, "Double-click", "Fly to that point", KeyGroup::Camera},
    {Action::RecallBookmark, Key::Num1, Modifier::None, "1 .. 9", "Go to bookmark", KeyGroup::Camera},
    {Action::SaveBookmark, Key::Num1, Modifier::Ctrl, "Ctrl + 1 .. 9", "Save bookmark", KeyGroup::Camera},
    {Action::Reroll, Key::R, Modifier::None, "R", "Reroll: new seed", KeyGroup::Planet},
    {Action::Atmosphere, Key::H, Modifier::None, "H", "Atmosphere on / off", KeyGroup::Planet},
    {Action::HidePanel, Key::Tab, Modifier::None, "Tab", "Hide / show the Planet window", KeyGroup::View},
    {Action::Help, Key::F1, Modifier::None, "F1", "This help", KeyGroup::View},
    {Action::Turntable, Key::F5, Modifier::None, "F5", "Play / stop the turntable", KeyGroup::View},
    {Action::Screenshot, Key::F12, Modifier::None, "F12", "Screenshot (PNG)", KeyGroup::View},
    {Action::Escape,
     Key::Escape,
     Modifier::None,
     "Esc",
     "Close help, stop turntable, hold 1 s to quit",
     KeyGroup::View},
}};

constexpr const KeyBind& Find(Action action)
{
    for (const KeyBind& row : Table)
        if (row.action == action)
            return row;
    return Table[0];
}

constexpr const char* GroupName(KeyGroup group)
{
    switch (group)
    {
    case KeyGroup::Camera:
        return "Camera";
    case KeyGroup::Planet:
        return "Planet";
    case KeyGroup::View:
        return "View";
    }
    return "";
}

// True when another row has the same key with Ctrl (digits: plain recalls, Ctrl saves).
constexpr bool HasCtrlTwin(const KeyBind& row)
{
    if (row.key == Key::None || row.modifier == Modifier::Ctrl)
        return false;
    for (const KeyBind& other : Table)
        if (other.key == row.key && other.modifier == Modifier::Ctrl)
            return true;
    return false;
}

// Ctrl is also the slow-fly key, so only rows that need it look at it: a Ctrl row wants it
// down, a row with a Ctrl twin wants it up, any other row ignores it.
constexpr bool ModifierMatches(bool ctrlDown, Modifier modifier, bool hasCtrlTwin)
{
    if (modifier == Modifier::Ctrl)
        return ctrlDown;
    return !(hasCtrlTwin && ctrlDown);
}

inline bool ModifierMatches(const Input& input, const KeyBind& row)
{
    const bool ctrlDown = input.IsKeyDown(Slower) || input.IsKeyDown(RightCtrl);
    return ModifierMatches(ctrlDown, row.modifier, HasCtrlTwin(row));
}

// True on the frame the row's key goes down with its modifier.
inline bool Pressed(const Input& input, Action action)
{
    const KeyBind& row = Find(action);
    return row.key != Key::None && ModifierMatches(input, row) && input.IsKeyPressed(row.key);
}

// 1..9 for the digit rows (RecallBookmark, SaveBookmark); 0 when none went down.
inline int DigitPressed(const Input& input, Action action)
{
    const KeyBind& row = Find(action);
    if (!ModifierMatches(input, row))
        return 0;
    for (int i = 0; i < 9; ++i)
        if (input.IsKeyPressed(static_cast<Key>(static_cast<int>(row.key) + i)))
            return i + 1;
    return 0;
}

} // namespace Keybinds

} // namespace planets::app
