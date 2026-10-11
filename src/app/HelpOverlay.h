#pragma once

namespace planets::app
{

// F1 key list, built from the Keybinds table. Opens by itself on the first launch.
class HelpOverlay
{
public:
    // Reads gui_state.json (working directory, like imgui.ini). A missing file means first
    // launch: the overlay opens and the file is written so it stays closed next time.
    void LoadFirstLaunchState();

    void Toggle() { _open = !_open; }
    void Close() { _open = false; }
    bool IsOpen() const { return _open; }
    bool* OpenFlag() { return &_open; }

    void Draw(); // call inside an ImGui frame

private:
    bool _open = false;
};

} // namespace planets::app
