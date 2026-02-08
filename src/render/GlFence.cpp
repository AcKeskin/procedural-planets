#include "GlFence.h"

namespace planets::render {

GlFence::~GlFence()
{
    Reset();
}

GlFence::GlFence(GlFence&& other) noexcept
    : _sync(other._sync)
{
    other._sync = nullptr;
}

GlFence& GlFence::operator=(GlFence&& other) noexcept
{
    if (this != &other)
    {
        Reset();
        _sync = other._sync;
        other._sync = nullptr;
    }
    return *this;
}

void GlFence::Place()
{
    Reset();
    _sync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
}

bool GlFence::IsSignaled() const
{
    if (!_sync)
        return false;

    GLenum result = glClientWaitSync(_sync, 0, 0);
    return result == GL_ALREADY_SIGNALED || result == GL_CONDITION_SATISFIED;
}

void GlFence::Reset()
{
    if (_sync)
    {
        glDeleteSync(_sync);
        _sync = nullptr;
    }
}

} // namespace planets::render
