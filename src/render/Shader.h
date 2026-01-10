#pragma once

#include <string>
#include <glm/glm.hpp>

namespace planets::render {

class Shader
{
public:
    Shader();
    ~Shader();

    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    bool LoadFromFiles(const std::string& vertexPath, const std::string& fragmentPath);
    void Use() const;

    void SetInt(const std::string& name, int value) const;
    void SetFloat(const std::string& name, float value) const;
    void SetVec3(const std::string& name, const glm::vec3& value) const;
    void SetMat4(const std::string& name, const glm::mat4& value) const;

    unsigned int GetProgram() const { return _program; }

private:
    bool CompileShader(unsigned int& shader, unsigned int type, const std::string& source);
    std::string LoadFile(const std::string& path);

    unsigned int _program;
};

} // namespace planets::render
