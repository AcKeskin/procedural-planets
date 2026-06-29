#include "WindowedGLBackend.h"
#include <GL/gl3w.h>
#include <iostream>

namespace planetgen
{

bool WindowedGLBackend::Init()
{
    // gl3w function pointers are per-module on Windows — must init even when sharing a context
    if (gl3wInit())
    {
        std::cerr << "[planetgen] gl3w init failed (existing context)" << std::endl;
        return false;
    }
    return true;
}

uint32_t WindowedGLBackend::CompileShader(const std::string& source)
{
    return _device.CompileProgram(source);
}

void WindowedGLBackend::DestroyShader(uint32_t shader)
{
    _device.DestroyProgram(shader);
}

void WindowedGLBackend::BindShader(uint32_t shader)
{
    _device.BindProgram(shader);
}

GpuBufferHandle WindowedGLBackend::CreateBuffer(size_t sizeBytes)
{
    return _device.CreateBuffer(sizeBytes);
}

void WindowedGLBackend::UploadBuffer(GpuBufferHandle buffer, const void* data, size_t sizeBytes)
{
    _device.UploadBuffer(buffer, data, sizeBytes);
}

void WindowedGLBackend::DownloadBuffer(GpuBufferHandle buffer, void* data, size_t sizeBytes)
{
    _device.DownloadBuffer(buffer, data, sizeBytes);
}

void WindowedGLBackend::DestroyBuffer(GpuBufferHandle buffer)
{
    _device.DestroyBuffer(buffer);
}

void WindowedGLBackend::BindBuffer(GpuBufferHandle buffer, uint32_t bindingPoint)
{
    _device.BindBuffer(buffer, bindingPoint);
}

GpuBufferHandle WindowedGLBackend::CreateParamBuffer(size_t sizeBytes)
{
    return _device.CreateUBO(sizeBytes);
}

void WindowedGLBackend::SetParams(GpuBufferHandle ubo, const void* data, size_t sizeBytes, uint32_t bindingPoint)
{
    _device.UploadUBO(ubo, data, sizeBytes);
    _device.BindUBO(ubo, bindingPoint);
}

void WindowedGLBackend::DestroyParamBuffer(GpuBufferHandle ubo)
{
    _device.DestroyUBO(ubo);
}

void WindowedGLBackend::Dispatch(uint32_t groupsX, uint32_t groupsY, uint32_t groupsZ)
{
    _device.Dispatch(groupsX, groupsY, groupsZ);
}

void WindowedGLBackend::Barrier()
{
    _device.Barrier();
}

GpuTextureHandle WindowedGLBackend::CreateTexture3D_RG32F(uint32_t resolution)
{
    return _device.CreateTexture3D_RG32F(resolution);
}

void WindowedGLBackend::BindImage3D(GpuTextureHandle texture, uint32_t imageUnit)
{
    _device.BindImage3D(texture, imageUnit);
}

void WindowedGLBackend::BindTexture3D(GpuTextureHandle texture, uint32_t samplerUnit)
{
    _device.BindTexture3D(texture, samplerUnit);
}

void WindowedGLBackend::DestroyTexture(GpuTextureHandle texture)
{
    _device.DestroyTexture(texture);
}

void WindowedGLBackend::ImageBarrier()
{
    _device.ImageBarrier();
}

} // namespace planetgen
