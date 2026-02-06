#pragma once

namespace planets::core {
class Camera;
}

namespace planets::render {

class DebugPanel
{
public:
    void Draw(const planets::core::Camera& camera, float& moveSpeed, bool& visible);
};

} // namespace planets::render
