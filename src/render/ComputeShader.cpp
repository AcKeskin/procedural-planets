#include "ComputeShader.h"
#include <GL/gl3w.h>
#include <glm/gtc/type_ptr.hpp>
#include <fstream>
#include <sstream>
#include <iostream>

namespace planets::render {

ComputeShader::ComputeShader()
    : _program(0)
{
}

ComputeShader::~ComputeShader()
{
    if (_program)
    {
        glDeleteProgram(_program);
    }
}

ComputeShader::ComputeShader(ComputeShader&& other) noexcept
    : _program(other._program)
{
    other._program = 0;
}

ComputeShader& ComputeShader::operator=(ComputeShader&& other) noexcept
{
    if (this != &other)
    {
        if (_program)
        {
            glDeleteProgram(_program);
        }
        _program = other._program;
        other._program = 0;
    }
    return *this;
}

bool ComputeShader::LoadFromFile(const std::string& path)
{
    std::string source = LoadFile(path);
    if (source.empty())
    {
        return false;
    }

    // Compile compute shader
    unsigned int shader = glCreateShader(GL_COMPUTE_SHADER);
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cerr << "[ComputeShader] Compile error in " << path << ": " << infoLog << std::endl;
        glDeleteShader(shader);
        return false;
    }

    // Link program
    _program = glCreateProgram();
    glAttachShader(_program, shader);
    glLinkProgram(_program);

    glGetProgramiv(_program, GL_LINK_STATUS, &success);
    if (!success)
    {
        char infoLog[512];
        glGetProgramInfoLog(_program, 512, nullptr, infoLog);
        std::cerr << "[ComputeShader] Link error: " << infoLog << std::endl;
        glDeleteProgram(_program);
        _program = 0;
    }

    glDeleteShader(shader);
    return _program != 0;
}

void ComputeShader::Use() const
{
    glUseProgram(_program);
}

void ComputeShader::Dispatch(unsigned int groupsX, unsigned int groupsY, unsigned int groupsZ) const
{
    glDispatchCompute(groupsX, groupsY, groupsZ);
}

void ComputeShader::WaitForCompletion()
{
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

void ComputeShader::SetInt(const std::string& name, int value) const
{
    glUniform1i(glGetUniformLocation(_program, name.c_str()), value);
}

void ComputeShader::SetUint(const std::string& name, unsigned int value) const
{
    glUniform1ui(glGetUniformLocation(_program, name.c_str()), value);
}

void ComputeShader::SetFloat(const std::string& name, float value) const
{
    glUniform1f(glGetUniformLocation(_program, name.c_str()), value);
}

void ComputeShader::SetVec3(const std::string& name, const glm::vec3& value) const
{
    glUniform3fv(glGetUniformLocation(_program, name.c_str()), 1, glm::value_ptr(value));
}

void ComputeShader::SetVec4(const std::string& name, const glm::vec4& value) const
{
    glUniform4fv(glGetUniformLocation(_program, name.c_str()), 1, glm::value_ptr(value));
}

void ComputeShader::SetVec4Array(const std::string& name, const glm::vec4* values, int count) const
{
    glUniform4fv(glGetUniformLocation(_program, name.c_str()), count, glm::value_ptr(values[0]));
}

std::string ComputeShader::LoadFile(const std::string& path)
{
    std::ifstream file(path);
    if (!file.is_open())
    {
        std::cerr << "[ComputeShader] Failed to open: " << path << std::endl;
        return "";
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

} // namespace planets::render
