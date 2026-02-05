#pragma once

#include <string>
#include <unordered_set>
#include <glm/glm.hpp>

namespace planets::render {

class ComputeShader
{
public:
    ComputeShader();
    ~ComputeShader();

    ComputeShader(const ComputeShader&) = delete;
    ComputeShader& operator=(const ComputeShader&) = delete;

    ComputeShader(ComputeShader&& other) noexcept;
    ComputeShader& operator=(ComputeShader&& other) noexcept;

    bool LoadFromFile(const std::string& path);
    void Use() const;

    // Dispatch compute work
    void Dispatch(unsigned int groupsX, unsigned int groupsY = 1, unsigned int groupsZ = 1) const;

    // Memory barrier after compute operations
    static void WaitForCompletion();

    // Uniform setters
    void SetInt(const std::string& name, int value) const;
    void SetUint(const std::string& name, unsigned int value) const;
    void SetFloat(const std::string& name, float value) const;
    void SetVec3(const std::string& name, const glm::vec3& value) const;
    void SetVec4(const std::string& name, const glm::vec4& value) const;
    void SetVec4Array(const std::string& name, const glm::vec4* values, int count) const;

    unsigned int GetProgram() const { return _program; }
    bool IsValid() const { return _program != 0; }

private:
    std::string LoadFile(const std::string& path);
    std::string PreprocessIncludes(const std::string& source, const std::string& sourcePath, std::unordered_set<std::string>& includedFiles);

    unsigned int _program;
};

} // namespace planets::render
