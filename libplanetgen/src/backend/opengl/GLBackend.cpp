#include "GLBackend.h"
#include <GL/gl3w.h>
#include <GLFW/glfw3.h>
#include <iostream>

namespace planetgen
{

GLBackend::~GLBackend()
{
    if (_window)
    {
        glfwDestroyWindow(_window);
        glfwTerminate();
        _window = nullptr;
    }
}

bool GLBackend::Init()
{
    if (!_ownsContext)
    {
        // gl3w function pointers are per-module on Windows — must init even when sharing a context
        if (gl3wInit())
        {
            std::cerr << "[planetgen] gl3w init failed (existing context)" << std::endl;
            return false;
        }
        return true;
    }

    if (!glfwInit())
    {
        std::cerr << "[planetgen] GLFW init failed" << std::endl;
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

    _window = glfwCreateWindow(1, 1, "planetgen_headless", nullptr, nullptr);
    if (!_window)
    {
        std::cerr << "[planetgen] Failed to create headless GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(_window);

    if (gl3wInit())
    {
        std::cerr << "[planetgen] gl3w init failed" << std::endl;
        glfwDestroyWindow(_window);
        glfwTerminate();
        _window = nullptr;
        return false;
    }

    return true;
}

uint32_t GLBackend::CompileShader(const std::string& source)
{
    return _device.CompileProgram(source);
}

void GLBackend::DestroyShader(uint32_t shader)
{
    _device.DestroyProgram(shader);
}

void GLBackend::BindShader(uint32_t shader)
{
    _device.BindProgram(shader);
}

GpuBufferHandle GLBackend::CreateBuffer(size_t sizeBytes)
{
    return _device.CreateBuffer(sizeBytes);
}

void GLBackend::UploadBuffer(GpuBufferHandle buffer, const void* data, size_t sizeBytes)
{
    _device.UploadBuffer(buffer, data, sizeBytes);
}

void GLBackend::DownloadBuffer(GpuBufferHandle buffer, void* data, size_t sizeBytes)
{
    _device.DownloadBuffer(buffer, data, sizeBytes);
}

void GLBackend::DestroyBuffer(GpuBufferHandle buffer)
{
    _device.DestroyBuffer(buffer);
}

void GLBackend::BindBuffer(GpuBufferHandle buffer, uint32_t bindingPoint)
{
    _device.BindBuffer(buffer, bindingPoint);
}

GpuBufferHandle GLBackend::CreateParamBuffer(size_t sizeBytes)
{
    return _device.CreateUBO(sizeBytes);
}

void GLBackend::SetParams(GpuBufferHandle ubo, const void* data, size_t sizeBytes, uint32_t bindingPoint)
{
    _device.UploadUBO(ubo, data, sizeBytes);
    _device.BindUBO(ubo, bindingPoint);
}

void GLBackend::DestroyParamBuffer(GpuBufferHandle ubo)
{
    _device.DestroyUBO(ubo);
}

void GLBackend::Dispatch(uint32_t groupsX, uint32_t groupsY, uint32_t groupsZ)
{
    _device.Dispatch(groupsX, groupsY, groupsZ);
}

void GLBackend::Barrier()
{
    _device.Barrier();
}

GpuTextureHandle GLBackend::CreateTexture3D_RG32F(uint32_t resolution)
{
    return _device.CreateTexture3D_RG32F(resolution);
}

void GLBackend::BindImage3D(GpuTextureHandle texture, uint32_t imageUnit)
{
    _device.BindImage3D(texture, imageUnit);
}

void GLBackend::BindTexture3D(GpuTextureHandle texture, uint32_t samplerUnit)
{
    _device.BindTexture3D(texture, samplerUnit);
}

void GLBackend::DestroyTexture(GpuTextureHandle texture)
{
    _device.DestroyTexture(texture);
}

void GLBackend::ImageBarrier()
{
    _device.ImageBarrier();
}

void GLBackend::Finish()
{
    _device.Finish();
}

GpuTextureHandle GLBackend::CreateTexture2DArray_RGBA32F(uint32_t resolution, uint32_t layers)
{
    return _device.CreateTexture2DArray_RGBA32F(resolution, layers);
}

void GLBackend::BindImage2DArray(GpuTextureHandle texture, uint32_t imageUnit)
{
    _device.BindImage2DArray(texture, imageUnit);
}

void GLBackend::BindTexture2DArray(GpuTextureHandle texture, uint32_t samplerUnit)
{
    _device.BindTexture2DArray(texture, samplerUnit);
}

void GLBackend::DownloadTexture(GpuTextureHandle texture, void* data, size_t sizeBytes)
{
    _device.DownloadTexture(texture, data, sizeBytes);
}

} // namespace planetgen
