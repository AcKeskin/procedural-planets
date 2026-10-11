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
    void DrawContent(const planets::core::Camera& camera, float& moveSpeed, bool autoOrbit, float& autoOrbitSpeed);
};

} // namespace planets::render
