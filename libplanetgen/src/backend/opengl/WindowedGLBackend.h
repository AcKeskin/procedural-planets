#pragma once

#include "../IComputeBackend.h"
#include "GlDevice.h"

namespace planetgen
{

// Wraps an existing GL context on the calling thread.
// Caller owns the context lifetime — this class only inits gl3w.
class WindowedGLBackend : public IComputeBackend
{
public:
    WindowedGLBackend() = default;
    ~WindowedGLBackend() override = default;

    bool Init();

    // IComputeBackend
    uint32_t CompileShader(const std::string& source) override;
    void DestroyShader(uint32_t shader) override;
    void BindShader(uint32_t shader) override;

    GpuBufferHandle CreateBuffer(size_t sizeBytes) override;
    void UploadBuffer(GpuBufferHandle buffer, const void* data, size_t sizeBytes) override;
    void DownloadBuffer(GpuBufferHandle buffer, void* data, size_t sizeBytes) override;
    void DestroyBuffer(GpuBufferHandle buffer) override;
    void BindBuffer(GpuBufferHandle buffer, uint32_t bindingPoint) override;

    void SetParams(GpuBufferHandle ubo, const void* data, size_t sizeBytes, uint32_t bindingPoint) override;
    GpuBufferHandle CreateParamBuffer(size_t sizeBytes) override;
    void DestroyParamBuffer(GpuBufferHandle ubo) override;

    void Dispatch(uint32_t groupsX, uint32_t groupsY, uint32_t groupsZ) override;
    void Barrier() override;

    GlDevice& Device() { return _device; }

private:
    GlDevice _device;
};

} // namespace planetgen
