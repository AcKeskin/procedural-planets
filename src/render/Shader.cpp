#include "Shader.h"
#include <GL/gl3w.h>
#include <glm/gtc/type_ptr.hpp>
#include <fstream>
#include <sstream>
#include <iostream>

namespace planets::render {

Shader::Shader()
    : _program(0)
{
}

Shader::~Shader()
{
    if (_program)
    {
        glDeleteProgram(_program);
    }
}

bool Shader::LoadFromFiles(const std::string& vertexPath, const std::string& fragmentPath)
{
    std::string vertexSource = LoadFile(vertexPath);
    std::string fragmentSource = LoadFile(fragmentPath);

    if (vertexSource.empty() || fragmentSource.empty())
    {
        return false;
    }

    unsigned int vertexShader, fragmentShader;
    if (!CompileShader(vertexShader, GL_VERTEX_SHADER, vertexSource))
    {
        return false;
    }

    if (!CompileShader(fragmentShader, GL_FRAGMENT_SHADER, fragmentSource))
    {
        glDeleteShader(vertexShader);
        return false;
    }

    _program = glCreateProgram();
    glAttachShader(_program, vertexShader);
    glAttachShader(_program, fragmentShader);
    glLinkProgram(_program);

    int success;
    glGetProgramiv(_program, GL_LINK_STATUS, &success);
    if (!success)
    {
        char infoLog[512];
        glGetProgramInfoLog(_program, 512, nullptr, infoLog);
        std::cerr << "[Shader] Link error: " << infoLog << std::endl;
        glDeleteProgram(_program);
        _program = 0;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return _program != 0;
}

void Shader::Use() const
{
    glUseProgram(_program);
}

void Shader::SetInt(const std::string& name, int value) const
{
    glUniform1i(glGetUniformLocation(_program, name.c_str()), value);
}

void Shader::SetFloat(const std::string& name, float value) const
{
    glUniform1f(glGetUniformLocation(_program, name.c_str()), value);
}

void Shader::SetVec3(const std::string& name, const glm::vec3& value) const
{
    glUniform3fv(glGetUniformLocation(_program, name.c_str()), 1, glm::value_ptr(value));
}

void Shader::SetMat4(const std::string& name, const glm::mat4& value) const
{
    glUniformMatrix4fv(glGetUniformLocation(_program, name.c_str()), 1, GL_FALSE, glm::value_ptr(value));
}

bool Shader::CompileShader(unsigned int& shader, unsigned int type, const std::string& source)
{
    shader = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cerr << "[Shader] Compile error: " << infoLog << std::endl;
        glDeleteShader(shader);
        return false;
    }

    return true;
}

std::string Shader::LoadFile(const std::string& path)
{
    std::ifstream file(path);
    if (!file.is_open())
    {
        std::cerr << "[Shader] Failed to open: " << path << std::endl;
        return "";
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

} // namespace planets::render
