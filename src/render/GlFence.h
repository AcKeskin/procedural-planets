#pragma once

#include <GL/gl3w.h>

namespace planets::render
{

// RAII wrapper around GLsync for non-blocking GPU completion checks
class GlFence
{
public:
    GlFence() = default;
    ~GlFence();

    GlFence(const GlFence&) = delete;
    GlFence& operator=(const GlFence&) = delete;

    GlFence(GlFence&& other) noexcept;
    GlFence& operator=(GlFence&& other) noexcept;

    // Insert fence after current GPU commands
    void Place();

    // Non-blocking check: true if all commands before Place() have completed
    bool IsSignaled() const;

    // Delete the fence object
    void Reset();

    bool IsValid() const { return _sync != nullptr; }

private:
    GLsync _sync = nullptr;
};

} // namespace planets::render
