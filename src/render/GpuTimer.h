#pragma once

#include <array>
#include <cstdint>

namespace planets::render
{

// Measures GPU time of a pass with GL_TIME_ELAPSED queries. A small ring of queries is read
// back only once its result is available, so timing never stalls the pipeline; the reading
// lags a few frames and is smoothed.
class GpuTimer
{
public:
    GpuTimer() = default;
    ~GpuTimer();

    GpuTimer(const GpuTimer&) = delete;
    GpuTimer& operator=(const GpuTimer&) = delete;

    void Begin();
    void End();

    // Smoothed GPU time of the pass, in milliseconds (0 until the first result lands).
    float Milliseconds() const { return _smoothedMs; }

private:
    static constexpr int kRing = 4;

    void Collect();

    std::array<uint32_t, kRing> _queries{};
    std::array<bool, kRing> _pending{};
    int _next = 0;
    bool _created = false;
    float _smoothedMs = 0.0f;
};

} // namespace planets::render
