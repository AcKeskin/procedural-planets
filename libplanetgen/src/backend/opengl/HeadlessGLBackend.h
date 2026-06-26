#pragma once

#include "../IComputeBackend.h"
#include "GlDevice.h"

struct GLFWwindow;

namespace planetgen
{

// Creates a hidden 1x1 GLFW window to own a GL context for offscreen compute.
// Owns both the window and gl context — destroys them in the destructor.
class HeadlessGLBackend : public IComputeBackend
{
public:
    HeadlessGLBackend() = default;
    ~HeadlessGLBackend() override;

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
    GLFWwindow* _window = nullptr;
    GlDevice _device;
};

} // namespace planetgen
