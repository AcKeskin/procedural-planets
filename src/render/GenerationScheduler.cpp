#include "GenerationScheduler.h"
#include "lod/QuadTreeNode.h"
#include <algorithm>
#include <iostream>

namespace planets::render
{

void GenerationScheduler::Initialize(PgBody body, uint32_t seed)
{
    CancelAll();
    _body = body;
    _seed = seed;
}

void GenerationScheduler::SetBody(PgBody body, uint32_t seed)
{
    _body = body;
    _seed = seed;
}

void GenerationScheduler::Enqueue(GenerationRequest request)
{
    GenerationTask task;
    task.request = std::move(request);
    _pending.push_back(std::move(task));
}

void GenerationScheduler::ProcessFrame(int maxDispatches, int maxCompletions)
{
    // Phase 1: Check in-flight fences, readback completed tasks
    int completions = 0;
    for (auto it = _inFlight.begin(); it != _inFlight.end() && completions < maxCompletions;)
    {
        if (it->fence.IsSignaled())
        {
            CompleteTask(*it);
            it = _inFlight.erase(it);
            ++completions;
        }
        else
        {
            ++it;
        }
    }

    // Phase 2: Sort pending by priority (closest patches first)
    std::sort(_pending.begin(),
              _pending.end(),
              [](const GenerationTask& a, const GenerationTask& b) { return a.request.priority < b.request.priority; });

    // Phase 3: Dispatch new tasks up to budget
    int dispatches = 0;
    while (!_pending.empty() && dispatches < maxDispatches)
    {
        auto task = std::move(_pending.front());
        _pending.erase(_pending.begin());

        DispatchTask(task);
        _inFlight.push_back(std::move(task));
        ++dispatches;
    }
}

void GenerationScheduler::DispatchTask(GenerationTask& task)
{
    const auto& unitVertices = task.request.patch->GetUnitSphereVertices();
    task.vertexCount = unitVertices.size();

    // Pack vertices as flat float array
    std::vector<float> packedVertices;
    packedVertices.reserve(task.vertexCount * 3);
    for (const auto& v : unitVertices)
    {
        packedVertices.push_back(v.x);
        packedVertices.push_back(v.y);
        packedVertices.push_back(v.z);
    }

    task.vertexBuffer.Upload(packedVertices);
    task.heightBuffer.Allocate(task.vertexCount);
    task.normalBuffer.Allocate(task.vertexCount * 3);
    task.shadingBuffer.Allocate(task.vertexCount);

    // The library writes per-vertex data into these buffers GPU-resident and
    // returns without syncing; the fence below is our completion signal. No erosion
    // on this path — it is neighbour-coupled and would seam at patch boundaries.
    pg_generate_patch(_body,
                      packedVertices.data(),
                      static_cast<uint32_t>(task.vertexCount),
                      _seed,
                      task.heightBuffer.GetHandle(),
                      task.normalBuffer.GetHandle(),
                      task.shadingBuffer.GetHandle(),
                      nullptr);

    // Single fence covers the dispatch; the app owns completion tracking.
    task.fence.Place();
}

void GenerationScheduler::CompleteTask(GenerationTask& task)
{
    // Readback height data from GPU
    std::vector<float> heights;
    task.heightBuffer.Download(heights);

    // Readback analytical normals from GPU
    std::vector<float> packedNormals;
    task.normalBuffer.Download(packedNormals);
    std::vector<glm::vec3> computedNormals(task.vertexCount);
    for (size_t i = 0; i < task.vertexCount; ++i)
    {
        computedNormals[i] = glm::vec3(packedNormals[i * 3], packedNormals[i * 3 + 1], packedNormals[i * 3 + 2]);
    }

    // Readback shading data from GPU
    std::vector<glm::vec4> shadingData;
    if (task.shadingBuffer.IsValid())
    {
        task.shadingBuffer.Download(shadingData);
    }
    else
    {
        shadingData.resize(task.vertexCount, glm::vec4(0.0f));
    }

    // Build mesh on the patch using GPU-computed analytical normals
    task.request.patch->GenerateMesh(heights, shadingData, computedNormals);

    // Move to completed results
    _completed.push_back({task.request.targetNode, task.request.type, std::move(task.request.patch)});
}

std::vector<CompletedGeneration> GenerationScheduler::TakeCompleted()
{
    std::vector<CompletedGeneration> result;
    result.swap(_completed);
    return result;
}

void GenerationScheduler::CancelAll()
{
    // In-flight tasks: fences and buffers cleaned up by RAII destructors
    _pending.clear();
    _inFlight.clear();
    _completed.clear();
}

bool GenerationScheduler::HasPendingWork() const
{
    return !_pending.empty() || !_inFlight.empty();
}

} // namespace planets::render
