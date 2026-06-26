#pragma once

#include "GlFence.h"
#include "GpuBuffer.h"
#include "TerrainGenerator.h"
#include "BodyRuntime.h"
#include "lod/SpherePatch.h"
#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include <cstdint>

namespace planets::render::lod
{
class QuadTreeNode;
}

namespace planets::render
{

enum class GenerationType
{
    Initial,
    Split,
    Merge
};

struct GenerationRequest
{
    lod::QuadTreeNode* targetNode = nullptr;
    GenerationType type = GenerationType::Initial;
    std::unique_ptr<lod::SpherePatch> patch;
    float priority = 0.0f; // Lower = higher priority (distance to camera)
};

// Tracks a single in-flight GPU generation (height + shading compute)
struct GenerationTask
{
    GenerationRequest request;

    // Per-task GPU buffers so multiple dispatches can coexist
    GpuBuffer<float> vertexBuffer;
    GpuBuffer<float> heightBuffer;
    GpuBuffer<float> normalBuffer;
    GpuBuffer<float> erosionScratchBuffer;
    GpuBuffer<glm::vec4> shadingBuffer;

    GlFence fence;
    size_t vertexCount = 0;
};

struct CompletedGeneration
{
    lod::QuadTreeNode* targetNode = nullptr;
    GenerationType type = GenerationType::Initial;
    std::unique_ptr<lod::SpherePatch> patch;
};

// Async generation pipeline with priority queue and per-frame budget.
// Dispatches compute shaders without blocking, checks GL fences for completion,
// and delivers finished patches for tree integration.
class GenerationScheduler
{
public:
    GenerationScheduler() = default;
    ~GenerationScheduler() = default;

    GenerationScheduler(const GenerationScheduler&) = delete;
    GenerationScheduler& operator=(const GenerationScheduler&) = delete;

    void Initialize(TerrainGenerator& terrainGen, const BodyRuntime& body, uint32_t seed);

    // Update body reference without full reinit (e.g. parameter slider change)
    void SetBody(const BodyRuntime& body, uint32_t seed);

    void Enqueue(GenerationRequest request);

    // Per-frame processing: check fences, readback completed, dispatch new work
    void ProcessFrame(int maxDispatches = 4, int maxCompletions = 4);

    // Retrieve completed patches for tree integration
    std::vector<CompletedGeneration> TakeCompleted();

    // Cancel all pending and in-flight work (before tree rebuild)
    void CancelAll();

    bool HasPendingWork() const;
    int GetPendingCount() const { return static_cast<int>(_pending.size()); }
    int GetInFlightCount() const { return static_cast<int>(_inFlight.size()); }

private:
    void DispatchTask(GenerationTask& task);
    void CompleteTask(GenerationTask& task);

    std::vector<GenerationTask> _pending;
    std::vector<GenerationTask> _inFlight;
    std::vector<CompletedGeneration> _completed;

    TerrainGenerator* _terrainGen = nullptr;
    const BodyRuntime* _body = nullptr;
    uint32_t _seed = 0;
};

} // namespace planets::render
