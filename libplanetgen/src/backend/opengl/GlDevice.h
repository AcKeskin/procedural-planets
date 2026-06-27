#pragma once

#include "../IComputeBackend.h"
#include <string>
#include <cstddef>
#include <cstdint>

namespace planetgen
{

// All raw GL operations — compile/link programs, buffers, UBOs, dispatch.
// Backend classes own only context creation; everything else delegates here.
class GlDevice
{
public:
    GlDevice() = default;
    ~GlDevice() = default;

    GlDevice(const GlDevice&) = delete;
    GlDevice& operator=(const GlDevice&) = delete;

    // Compile GLSL source into a linked program object. Returns 0 on failure.
    uint32_t CompileProgram(const std::string& source);
    void DestroyProgram(uint32_t program);

    void BindProgram(uint32_t program);
    uint32_t ActiveProgram() const { return _activeProgram; }

    // SSBO management
    GpuBufferHandle CreateBuffer(size_t sizeBytes);
    void UploadBuffer(GpuBufferHandle buffer, const void* data, size_t sizeBytes);
    void DownloadBuffer(GpuBufferHandle buffer, void* data, size_t sizeBytes);
    void DestroyBuffer(GpuBufferHandle buffer);
    void BindBuffer(GpuBufferHandle buffer, uint32_t bindingPoint);

    // UBO upload + bind to a named binding slot
    GpuBufferHandle CreateUBO(size_t sizeBytes);
    void UploadUBO(GpuBufferHandle ubo, const void* data, size_t sizeBytes);
    void BindUBO(GpuBufferHandle ubo, uint32_t bindingPoint);
    void DestroyUBO(GpuBufferHandle ubo);

    // Loose uniform setters — still needed for any shader that hasn't migrated to UBOs
    void SetInt(const std::string& name, int value);
    void SetUint(const std::string& name, uint32_t value);
    void SetFloat(const std::string& name, float value);
    void SetVec3(const std::string& name, const float* value);

    // 3D-texture (RG32F) support — continent mask: growth pass writes via image,
    // height stage reads via sampler.
    GpuTextureHandle CreateTexture3D_RG32F(uint32_t resolution);
    void BindImage3D(GpuTextureHandle texture, uint32_t imageUnit);
    void BindTexture3D(GpuTextureHandle texture, uint32_t samplerUnit);
    void DestroyTexture(GpuTextureHandle texture);

    void Dispatch(uint32_t groupsX, uint32_t groupsY = 1, uint32_t groupsZ = 1);
    void Barrier();
    void ImageBarrier();

private:
    int GetUniformLocation(const std::string& name);

    uint32_t _activeProgram = 0;
};

} // namespace planetgen
