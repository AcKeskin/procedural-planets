#include "HeadlessGLBackend.h"
#include <GL/gl3w.h>
#include <GLFW/glfw3.h>
#include <iostream>

namespace planetgen
{

HeadlessGLBackend::~HeadlessGLBackend()
{
    if (_window)
    {
        glfwDestroyWindow(_window);
        glfwTerminate();
        _window = nullptr;
    }
}

bool HeadlessGLBackend::Init()
{
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

uint32_t HeadlessGLBackend::CompileShader(const std::string& source)
{
    return _device.CompileProgram(source);
}

void HeadlessGLBackend::DestroyShader(uint32_t shader)
{
    _device.DestroyProgram(shader);
}

void HeadlessGLBackend::BindShader(uint32_t shader)
{
    _device.BindProgram(shader);
}

GpuBufferHandle HeadlessGLBackend::CreateBuffer(size_t sizeBytes)
{
    return _device.CreateBuffer(sizeBytes);
}

void HeadlessGLBackend::UploadBuffer(GpuBufferHandle buffer, const void* data, size_t sizeBytes)
{
    _device.UploadBuffer(buffer, data, sizeBytes);
}

void HeadlessGLBackend::DownloadBuffer(GpuBufferHandle buffer, void* data, size_t sizeBytes)
{
    _device.DownloadBuffer(buffer, data, sizeBytes);
}

void HeadlessGLBackend::DestroyBuffer(GpuBufferHandle buffer)
{
    _device.DestroyBuffer(buffer);
}

void HeadlessGLBackend::BindBuffer(GpuBufferHandle buffer, uint32_t bindingPoint)
{
    _device.BindBuffer(buffer, bindingPoint);
}

GpuBufferHandle HeadlessGLBackend::CreateParamBuffer(size_t sizeBytes)
{
    return _device.CreateUBO(sizeBytes);
}

void HeadlessGLBackend::SetParams(GpuBufferHandle ubo, const void* data, size_t sizeBytes, uint32_t bindingPoint)
{
    _device.UploadUBO(ubo, data, sizeBytes);
    _device.BindUBO(ubo, bindingPoint);
}

void HeadlessGLBackend::DestroyParamBuffer(GpuBufferHandle ubo)
{
    _device.DestroyUBO(ubo);
}

void HeadlessGLBackend::Dispatch(uint32_t groupsX, uint32_t groupsY, uint32_t groupsZ)
{
    _device.Dispatch(groupsX, groupsY, groupsZ);
}

void HeadlessGLBackend::Barrier()
{
    _device.Barrier();
}

} // namespace planetgen
