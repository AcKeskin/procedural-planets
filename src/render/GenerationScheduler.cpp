#include "GenerationScheduler.h"
#include "lod/QuadTreeNode.h"
#include <algorithm>
#include <iostream>

namespace planets::render
{

void GenerationScheduler::Initialize(TerrainGenerator& terrainGen,
                                     const EarthTerrainSettings& terrainSettings,
                                     const EarthShadingSettings& shadingSettings,
                                     uint32_t seed)
{
    CancelAll();
    _terrainGen = &terrainGen;
    _terrainSettings = &terrainSettings;
    _shadingSettings = &shadingSettings;
    _seed = seed;
}

void GenerationScheduler::SetSettings(const EarthTerrainSettings& terrainSettings,
                                      const EarthShadingSettings& shadingSettings,
                                      uint32_t seed)
{
    _terrainSettings = &terrainSettings;
    _shadingSettings = &shadingSettings;
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

    // Pack vertices as flat float array (same layout as synchronous path)
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

    // Dispatch height compute
    _terrainGen->DispatchHeightsAsync(task.vertexBuffer, task.heightBuffer, task.vertexCount, _seed, *_terrainSettings);

    // Erosion iterations (requires barrier between height and each iteration)
    if (_terrainSettings->enableErosion && _terrainGen->IsErosionReady())
    {
        int gridRes = task.request.patch->GetResolution();
        for (int i = 0; i < _terrainSettings->erosionIterations; ++i)
        {
            ComputeShader::WaitForCompletion();
            _terrainGen->DispatchErosionAsync(task.heightBuffer, task.vertexCount, gridRes, *_terrainSettings);
        }
    }

    // Barrier ensures height data (potentially eroded) is readable by shading
    ComputeShader::WaitForCompletion();

    // Dispatch shading compute (reads height buffer for climate model)
    if (_terrainGen->IsShadingReady())
    {
        task.shadingBuffer.Allocate(task.vertexCount);
        _terrainGen->DispatchShadingAsync(
            task.vertexBuffer, task.shadingBuffer, task.heightBuffer, task.vertexCount, _seed, *_shadingSettings, _terrainSettings->heightScale);
    }

    // Single fence covers all dispatches
    task.fence.Place();
}

void GenerationScheduler::CompleteTask(GenerationTask& task)
{
    // Readback height data from GPU
    std::vector<float> heights;
    task.heightBuffer.Download(heights);

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

    // Build mesh on the patch (CPU: normals, skirts, upload VAO)
    task.request.patch->GenerateMesh(heights, shadingData);

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
