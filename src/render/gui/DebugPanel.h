#pragma once

namespace planets::core
{
class Camera;
}

namespace planets::render
{

class DebugPanel
{
public:
    void
    Draw(const planets::core::Camera& camera, float& moveSpeed, bool autoOrbit, float& autoOrbitSpeed, bool& visible);
};

} // namespace planets::render
