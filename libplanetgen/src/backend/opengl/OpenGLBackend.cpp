#include "OpenGLBackend.h"
#include <GL/gl3w.h>
#include <GLFW/glfw3.h>
#include <iostream>

namespace planetgen
{

namespace
{
constexpr int ShaderInfoLogSize = 512;
} // namespace

OpenGLBackend::OpenGLBackend() = default;

OpenGLBackend::~OpenGLBackend()
{
    if (_ownsContext && _window)
    {
        glfwDestroyWindow(_window);
        glfwTerminate();
    }
}

bool OpenGLBackend::InitWithExistingContext()
{
    // gl3w function pointers are per-module on Windows — must init even with existing context
    if (gl3wInit())
    {
        std::cerr << "[planetgen] gl3w init failed (existing context)" << std::endl;
        return false;
    }

    _ownsContext = false;
    return true;
}

bool OpenGLBackend::InitHeadless()
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

    _ownsContext = true;
    return true;
}

uint32_t OpenGLBackend::CompileShader(const std::string& source)
{
    unsigned int shader = glCreateShader(GL_COMPUTE_SHADER);
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        char infoLog[ShaderInfoLogSize];
        glGetShaderInfoLog(shader, ShaderInfoLogSize, nullptr, infoLog);
        std::cerr << "[planetgen] Shader compile error: " << infoLog << std::endl;
        glDeleteShader(shader);
        return 0;
    }

    unsigned int program = glCreateProgram();
    glAttachShader(program, shader);
    glLinkProgram(program);

    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success)
    {
        char infoLog[ShaderInfoLogSize];
        glGetProgramInfoLog(program, ShaderInfoLogSize, nullptr, infoLog);
        std::cerr << "[planetgen] Shader link error: " << infoLog << std::endl;
        glDeleteProgram(program);
        program = 0;
    }

    glDeleteShader(shader);
    return program;
}

void OpenGLBackend::DestroyShader(uint32_t shader)
{
    if (shader)
        glDeleteProgram(shader);
}

void OpenGLBackend::BindShader(uint32_t shader)
{
    _activeProgram = shader;
    glUseProgram(shader);
}

GpuBufferHandle OpenGLBackend::CreateBuffer(size_t sizeBytes)
{
    unsigned int buffer;
    glGenBuffers(1, &buffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, static_cast<GLsizeiptr>(sizeBytes), nullptr, GL_DYNAMIC_COPY);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    return buffer;
}

void OpenGLBackend::UploadBuffer(GpuBufferHandle buffer, const void* data, size_t sizeBytes)
{
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, static_cast<GLsizeiptr>(sizeBytes), data, GL_DYNAMIC_COPY);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void OpenGLBackend::DownloadBuffer(GpuBufferHandle buffer, void* data, size_t sizeBytes)
{
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffer);
    glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, static_cast<GLsizeiptr>(sizeBytes), data);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void OpenGLBackend::DestroyBuffer(GpuBufferHandle buffer)
{
    if (buffer)
    {
        unsigned int buf = buffer;
        glDeleteBuffers(1, &buf);
    }
}

void OpenGLBackend::BindBuffer(GpuBufferHandle buffer, uint32_t bindingPoint)
{
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, bindingPoint, buffer);
}

int OpenGLBackend::GetUniformLocation(const std::string& name)
{
    return glGetUniformLocation(_activeProgram, name.c_str());
}

void OpenGLBackend::SetInt(const std::string& name, int value)
{
    glUniform1i(GetUniformLocation(name), value);
}

void OpenGLBackend::SetUint(const std::string& name, uint32_t value)
{
    glUniform1ui(GetUniformLocation(name), value);
}

void OpenGLBackend::SetFloat(const std::string& name, float value)
{
    glUniform1f(GetUniformLocation(name), value);
}

void OpenGLBackend::SetVec3(const std::string& name, const float* value)
{
    glUniform3fv(GetUniformLocation(name), 1, value);
}

void OpenGLBackend::Dispatch(uint32_t groupsX, uint32_t groupsY, uint32_t groupsZ)
{
    glDispatchCompute(groupsX, groupsY, groupsZ);
}

void OpenGLBackend::Barrier()
{
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

} // namespace planetgen
