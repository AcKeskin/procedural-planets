#pragma once

#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>

namespace planetgen
{

// GPU buffer handle — opaque to callers, meaningful to the backend
using GpuBufferHandle = uint32_t;

// Abstract compute backend interface
// Isolates shader management, buffer operations, and dispatch from the API layer
class IComputeBackend
{
public:
    virtual ~IComputeBackend() = default;

    // Compile a compute shader from GLSL source string. Returns 0 on failure.
    virtual uint32_t CompileShader(const std::string& source) = 0;
    virtual void DestroyShader(uint32_t shader) = 0;
    virtual void BindShader(uint32_t shader) = 0;

    // Buffer management
    virtual GpuBufferHandle CreateBuffer(size_t sizeBytes) = 0;
    virtual void UploadBuffer(GpuBufferHandle buffer, const void* data, size_t sizeBytes) = 0;
    virtual void DownloadBuffer(GpuBufferHandle buffer, void* data, size_t sizeBytes) = 0;
    virtual void DestroyBuffer(GpuBufferHandle buffer) = 0;
    virtual void BindBuffer(GpuBufferHandle buffer, uint32_t bindingPoint) = 0;

    // Uniform setters (shader must be bound)
    virtual void SetInt(const std::string& name, int value) = 0;
    virtual void SetUint(const std::string& name, uint32_t value) = 0;
    virtual void SetFloat(const std::string& name, float value) = 0;
    virtual void SetVec3(const std::string& name, const float* value) = 0;

    // Dispatch and synchronization
    virtual void Dispatch(uint32_t groupsX, uint32_t groupsY = 1, uint32_t groupsZ = 1) = 0;
    virtual void Barrier() = 0;
};

} // namespace planetgen
