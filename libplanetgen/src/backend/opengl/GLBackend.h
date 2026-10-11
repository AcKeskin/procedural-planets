#pragma once

#include "../IComputeBackend.h"
#include "GlDevice.h"

struct GLFWwindow;

namespace planetgen
{

// OpenGL compute backend. With ownsContext it creates a hidden 1x1 GLFW window for
// offscreen compute and destroys it in the destructor. Without, it runs on the
// caller's current context, whose lifetime the caller owns; only gl3w is inited.
class GLBackend : public IComputeBackend
{
public:
    explicit GLBackend(bool ownsContext) : _ownsContext(ownsContext) {}
    ~GLBackend() override;

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

    GpuTextureHandle CreateTexture3D_RG32F(uint32_t resolution) override;
    void BindImage3D(GpuTextureHandle texture, uint32_t imageUnit) override;
    void BindTexture3D(GpuTextureHandle texture, uint32_t samplerUnit) override;
    void DestroyTexture(GpuTextureHandle texture) override;

    GpuTextureHandle CreateTexture2DArray_RGBA32F(uint32_t resolution, uint32_t layers) override;
    void BindImage2DArray(GpuTextureHandle texture, uint32_t imageUnit) override;
    void BindTexture2DArray(GpuTextureHandle texture, uint32_t samplerUnit) override;
    void DownloadTexture(GpuTextureHandle texture, void* data, size_t sizeBytes) override;

    void Dispatch(uint32_t groupsX, uint32_t groupsY, uint32_t groupsZ) override;
    void Barrier() override;
    void ImageBarrier() override;
    void Finish() override;

    GlDevice& Device() { return _device; }

private:
    bool _ownsContext;
    GLFWwindow* _window = nullptr;
    GlDevice _device;
};

} // namespace planetgen
