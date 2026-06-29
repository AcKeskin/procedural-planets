#pragma once

#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>

namespace planetgen
{

// GPU buffer handle — opaque to callers, meaningful to the backend.
// Underlying type is a GL object name (uint32_t fits all GL names).
using GpuBufferHandle = uint32_t;

// GPU texture handle — same opaque-GL-name convention as GpuBufferHandle.
using GpuTextureHandle = uint32_t;

// Abstract compute backend.
// No GL types in this interface — consumers only see neutral handles and plain C types.
class IComputeBackend
{
public:
    virtual ~IComputeBackend() = default;

    // Compile a GLSL compute shader from source. Returns 0 on failure.
    virtual uint32_t CompileShader(const std::string& source) = 0;
    virtual void DestroyShader(uint32_t shader) = 0;
    virtual void BindShader(uint32_t shader) = 0;

    // SSBO management
    virtual GpuBufferHandle CreateBuffer(size_t sizeBytes) = 0;
    virtual void UploadBuffer(GpuBufferHandle buffer, const void* data, size_t sizeBytes) = 0;
    virtual void DownloadBuffer(GpuBufferHandle buffer, void* data, size_t sizeBytes) = 0;
    virtual void DestroyBuffer(GpuBufferHandle buffer) = 0;
    virtual void BindBuffer(GpuBufferHandle buffer, uint32_t bindingPoint) = 0;

    // Typed param struct upload via UBO.
    // All production shaders use this path — no loose setters on this interface.
    virtual GpuBufferHandle CreateParamBuffer(size_t sizeBytes) = 0;
    virtual void SetParams(GpuBufferHandle ubo, const void* data, size_t sizeBytes, uint32_t bindingPoint) = 0;
    virtual void DestroyParamBuffer(GpuBufferHandle ubo) = 0;

    // 3D-texture management (RG32F volumes — the continent mask). The growth pass
    // writes via the image binding; the height stage reads via the sampler binding.
    virtual GpuTextureHandle CreateTexture3D_RG32F(uint32_t resolution) = 0;
    virtual void BindImage3D(GpuTextureHandle texture, uint32_t imageUnit) = 0;       // write target
    virtual void BindTexture3D(GpuTextureHandle texture, uint32_t samplerUnit) = 0;   // sampler source
    virtual void DestroyTexture(GpuTextureHandle texture) = 0;

    // Dispatch and synchronization
    virtual void Dispatch(uint32_t groupsX, uint32_t groupsY = 1, uint32_t groupsZ = 1) = 0;
    virtual void Barrier() = 0;

    // Image/texture-fetch memory barrier (after the growth pass writes the mask,
    // before the height stage samples it). Distinct from the SSBO Barrier().
    virtual void ImageBarrier() = 0;
};

} // namespace planetgen
