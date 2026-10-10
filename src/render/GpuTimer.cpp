#include "GpuTimer.h"
#include <GL/gl3w.h>

namespace planets::render
{

namespace
{
constexpr float kSmoothing = 0.1f; // weight of the newest sample
constexpr double kNsPerMs = 1.0e6;
} // namespace

GpuTimer::~GpuTimer()
{
    if (_created)
        glDeleteQueries(kRing, _queries.data());
}

void GpuTimer::Begin()
{
    if (!_created)
    {
        glGenQueries(kRing, _queries.data());
        _created = true;
    }
    Collect();
    if (_pending[_next])
        return; // ring full: skip this frame rather than block on an old result
    glBeginQuery(GL_TIME_ELAPSED, _queries[_next]);
}

void GpuTimer::End()
{
    if (_pending[_next])
        return;
    glEndQuery(GL_TIME_ELAPSED);
    _pending[_next] = true;
    _next = (_next + 1) % kRing;
}

void GpuTimer::Collect()
{
    for (int i = 0; i < kRing; ++i)
    {
        if (!_pending[i])
            continue;
        GLint available = 0;
        glGetQueryObjectiv(_queries[i], GL_QUERY_RESULT_AVAILABLE, &available);
        if (!available)
            continue;
        GLuint64 ns = 0;
        glGetQueryObjectui64v(_queries[i], GL_QUERY_RESULT, &ns);
        _pending[i] = false;
        const float ms = static_cast<float>(static_cast<double>(ns) / kNsPerMs);
        _smoothedMs = (_smoothedMs == 0.0f) ? ms : _smoothedMs + (ms - _smoothedMs) * kSmoothing;
    }
}

} // namespace planets::render
