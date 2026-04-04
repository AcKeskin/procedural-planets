#pragma once

#include "../IComputeBackend.h"

struct GLFWwindow;

namespace planetgen
{

// OpenGL 4.5 compute backend
// Supports both existing-context and headless-context modes
class OpenGLBackend : public IComputeBackend
{
public:
    OpenGLBackend();
    ~OpenGLBackend() override;

    // Initialize with existing GL context on current thread
    bool InitWithExistingContext();

    // Initialize with a new headless GLFW window for offscreen compute
    bool InitHeadless();

    // IComputeBackend implementation
    uint32_t CompileShader(const std::string& source) override;
    void DestroyShader(uint32_t shader) override;
    void BindShader(uint32_t shader) override;

    GpuBufferHandle CreateBuffer(size_t sizeBytes) override;
    void UploadBuffer(GpuBufferHandle buffer, const void* data, size_t sizeBytes) override;
    void DownloadBuffer(GpuBufferHandle buffer, void* data, size_t sizeBytes) override;
    void DestroyBuffer(GpuBufferHandle buffer) override;
    void BindBuffer(GpuBufferHandle buffer, uint32_t bindingPoint) override;

    void SetInt(const std::string& name, int value) override;
    void SetUint(const std::string& name, uint32_t value) override;
    void SetFloat(const std::string& name, float value) override;
    void SetVec3(const std::string& name, const float* value) override;

    void Dispatch(uint32_t groupsX, uint32_t groupsY, uint32_t groupsZ) override;
    void Barrier() override;

private:
    int GetUniformLocation(const std::string& name);

    GLFWwindow* _window = nullptr; // Only set if we created a headless context
    uint32_t _activeProgram = 0;
    bool _ownsContext = false;
};

} // namespace planetgen
