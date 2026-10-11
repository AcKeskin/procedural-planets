#pragma once

#include "math/CameraController.h"
#include <glm/glm.hpp>
#include <array>
#include <string>

namespace planets::app
{

struct Bookmark
{
    bool used = false;
    core::CameraPose pose;
    glm::vec3 sunDir{1.0f, 0.0f, 0.0f}; // scene lightDir at the time
};

// Nine camera slots (keys 1..9), kept in bookmarks.json in the working directory.
class Bookmarks
{
public:
    static constexpr int SlotCount = 9;

    void Load(const std::string& path);
    bool Save(const std::string& path) const;

    // slot is 1..9
    void Set(int slot, const Bookmark& bookmark);
    const Bookmark& Get(int slot) const;

private:
    std::array<Bookmark, SlotCount> _slots;
};

} // namespace planets::app
