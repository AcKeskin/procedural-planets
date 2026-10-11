#pragma once

namespace planets::core
{
class Camera;
}

namespace planets::render
{

struct CinematicSettings;

// Planet window > Camera: view and fly speed, auto-rotate, and the F5 turntable.
class CameraTab
{
public:
    // flySpeed: multiplier on the altitude-based fly speed. play: set when the user starts the turntable.
    void Draw(core::Camera& camera, float& flySpeed, CinematicSettings& cinematic, bool& play);
};

} // namespace planets::render
