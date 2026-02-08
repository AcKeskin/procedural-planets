#include "Shader.h"
#include "GpuConstants.h"
#include <GL/gl3w.h>
#include <glm/gtc/type_ptr.hpp>
#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>
#include <regex>
#include <unordered_set>

namespace planets::render
{

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

    // Preprocess includes with cycle detection
    std::unordered_set<std::string> vertexIncluded;
    std::unordered_set<std::string> fragmentIncluded;

    vertexSource = PreprocessIncludes(vertexSource, vertexPath, vertexIncluded);
    fragmentSource = PreprocessIncludes(fragmentSource, fragmentPath, fragmentIncluded);

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
        char infoLog[ShaderInfoLogSize];
        glGetProgramInfoLog(_program, ShaderInfoLogSize, nullptr, infoLog);
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

void Shader::SetVec2(const std::string& name, const glm::vec2& value) const
{
    glUniform2fv(glGetUniformLocation(_program, name.c_str()), 1, glm::value_ptr(value));
}

void Shader::SetVec3(const std::string& name, const glm::vec3& value) const
{
    glUniform3fv(glGetUniformLocation(_program, name.c_str()), 1, glm::value_ptr(value));
}

void Shader::SetVec4(const std::string& name, const glm::vec4& value) const
{
    glUniform4fv(glGetUniformLocation(_program, name.c_str()), 1, glm::value_ptr(value));
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
        char infoLog[ShaderInfoLogSize];
        glGetShaderInfoLog(shader, ShaderInfoLogSize, nullptr, infoLog);
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

std::string Shader::PreprocessIncludes(const std::string& source,
                                       const std::string& sourcePath,
                                       std::unordered_set<std::string>& includedFiles)
{
    // Regex pattern to match #include "filename"
    static const std::regex includePattern(R"regex(^\s*#include\s+"([^"]+)"\s*$)regex");

    std::stringstream result;
    std::stringstream sourceStream(source);
    std::string line;

    // Get canonical path of current source file for cycle detection
    std::filesystem::path sourceFilePath(sourcePath);
    std::string canonicalSourcePath = std::filesystem::weakly_canonical(sourceFilePath).string();

    int lineNumber = 0;
    while (std::getline(sourceStream, line))
    {
        ++lineNumber;
        std::smatch match;

        if (std::regex_match(line, match, includePattern))
        {
            std::string includeFilename = match[1].str();

            // Resolve include path relative to shaders/includes/
            std::filesystem::path includePath = std::filesystem::path("shaders") / "includes" / includeFilename;
            std::string canonicalIncludePath;

            try
            {
                canonicalIncludePath = std::filesystem::weakly_canonical(includePath).string();
            }
            catch (const std::filesystem::filesystem_error& e)
            {
                std::cerr << "[Shader] Include resolution error in " << sourcePath << ":" << lineNumber << " - "
                          << e.what() << std::endl;
                return "";
            }

            // Check for circular dependency
            if (includedFiles.find(canonicalIncludePath) != includedFiles.end())
            {
                std::cerr << "[Shader] Circular include detected: " << canonicalIncludePath << " in " << sourcePath
                          << ":" << lineNumber << std::endl;
                return "";
            }

            // Load included file
            std::string includedSource = LoadFile(canonicalIncludePath);
            if (includedSource.empty())
            {
                std::cerr << "[Shader] Failed to load include: " << canonicalIncludePath << " referenced in "
                          << sourcePath << ":" << lineNumber << std::endl;
                return "";
            }

            // Mark this file as included to prevent cycles
            includedFiles.insert(canonicalIncludePath);

            // Recursively process includes in the included file
            std::string processedInclude = PreprocessIncludes(includedSource, canonicalIncludePath, includedFiles);
            if (processedInclude.empty() && !includedSource.empty())
            {
                // Preprocessing failed
                return "";
            }

            // Insert processed include content with line directive for debugging
            result << "// Begin include: " << includeFilename << "\n";
            result << processedInclude;
            result << "// End include: " << includeFilename << "\n";
        }
        else
        {
            // Regular line, pass through
            result << line << "\n";
        }
    }

    return result.str();
}

} // namespace planets::render
