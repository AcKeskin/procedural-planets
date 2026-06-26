#include "GlDevice.h"
#include <GL/gl3w.h>
#include <iostream>

namespace planetgen
{

namespace
{
constexpr int ShaderInfoLogSize = 1024;
} // namespace

uint32_t GlDevice::CompileProgram(const std::string& source)
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
        std::cerr << "[planetgen] Shader compile error:\n" << infoLog << std::endl;
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
        std::cerr << "[planetgen] Shader link error:\n" << infoLog << std::endl;
        glDeleteProgram(program);
        program = 0;
    }

    glDeleteShader(shader);
    return program;
}

void GlDevice::DestroyProgram(uint32_t program)
{
    if (program)
        glDeleteProgram(program);
}

void GlDevice::BindProgram(uint32_t program)
{
    _activeProgram = program;
    glUseProgram(program);
}

GpuBufferHandle GlDevice::CreateBuffer(size_t sizeBytes)
{
    unsigned int buffer;
    glGenBuffers(1, &buffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, static_cast<GLsizeiptr>(sizeBytes), nullptr, GL_DYNAMIC_COPY);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    return buffer;
}

void GlDevice::UploadBuffer(GpuBufferHandle buffer, const void* data, size_t sizeBytes)
{
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, static_cast<GLsizeiptr>(sizeBytes), data, GL_DYNAMIC_COPY);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void GlDevice::DownloadBuffer(GpuBufferHandle buffer, void* data, size_t sizeBytes)
{
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffer);
    glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, static_cast<GLsizeiptr>(sizeBytes), data);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void GlDevice::DestroyBuffer(GpuBufferHandle buffer)
{
    if (buffer)
    {
        unsigned int buf = buffer;
        glDeleteBuffers(1, &buf);
    }
}

void GlDevice::BindBuffer(GpuBufferHandle buffer, uint32_t bindingPoint)
{
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, bindingPoint, buffer);
}

GpuBufferHandle GlDevice::CreateUBO(size_t sizeBytes)
{
    unsigned int ubo;
    glGenBuffers(1, &ubo);
    glBindBuffer(GL_UNIFORM_BUFFER, ubo);
    glBufferData(GL_UNIFORM_BUFFER, static_cast<GLsizeiptr>(sizeBytes), nullptr, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    return ubo;
}

void GlDevice::UploadUBO(GpuBufferHandle ubo, const void* data, size_t sizeBytes)
{
    glBindBuffer(GL_UNIFORM_BUFFER, ubo);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, static_cast<GLsizeiptr>(sizeBytes), data);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void GlDevice::BindUBO(GpuBufferHandle ubo, uint32_t bindingPoint)
{
    glBindBufferBase(GL_UNIFORM_BUFFER, bindingPoint, ubo);
}

void GlDevice::DestroyUBO(GpuBufferHandle ubo)
{
    if (ubo)
    {
        unsigned int buf = ubo;
        glDeleteBuffers(1, &buf);
    }
}

int GlDevice::GetUniformLocation(const std::string& name)
{
    return glGetUniformLocation(_activeProgram, name.c_str());
}

void GlDevice::SetInt(const std::string& name, int value)
{
    glUniform1i(GetUniformLocation(name), value);
}

void GlDevice::SetUint(const std::string& name, uint32_t value)
{
    glUniform1ui(GetUniformLocation(name), value);
}

void GlDevice::SetFloat(const std::string& name, float value)
{
    glUniform1f(GetUniformLocation(name), value);
}

void GlDevice::SetVec3(const std::string& name, const float* value)
{
    glUniform3fv(GetUniformLocation(name), 1, value);
}

void GlDevice::Dispatch(uint32_t groupsX, uint32_t groupsY, uint32_t groupsZ)
{
    glDispatchCompute(groupsX, groupsY, groupsZ);
}

void GlDevice::Barrier()
{
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

} // namespace planetgen
