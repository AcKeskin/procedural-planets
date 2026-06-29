#include "ShaderRegistry.h"
#include <iostream>

namespace planetgen
{

ShaderRegistry::~ShaderRegistry()
{
    Clear();
}

void ShaderRegistry::SetCompiler(CompileFn compile, DestroyFn destroy)
{
    _compile = std::move(compile);
    _destroy = std::move(destroy);
}

bool ShaderRegistry::RegisterProgram(const std::string& key, const std::string& source)
{
    if (!_compile)
    {
        std::cerr << "[ShaderRegistry] No compiler set" << std::endl;
        return false;
    }

    // Evict old handle if re-registering under the same key
    auto it = _programs.find(key);
    if (it != _programs.end())
    {
        if (_destroy && it->second)
            _destroy(it->second);
        _programs.erase(it);
    }

    uint32_t handle = _compile(source);
    if (!handle)
    {
        std::cerr << "[ShaderRegistry] Compile failed for key: " << key << std::endl;
        return false;
    }

    _programs[key] = handle;
    return true;
}

bool ShaderRegistry::RegisterProgram(BuiltinProgram id, const std::string& source)
{
    return RegisterProgram(BuiltinKey(id), source);
}

uint32_t ShaderRegistry::Resolve(const std::string& key) const
{
    auto it = _programs.find(key);
    return (it != _programs.end()) ? it->second : 0u;
}

uint32_t ShaderRegistry::Resolve(BuiltinProgram id) const
{
    return Resolve(BuiltinKey(id));
}

void ShaderRegistry::Unregister(const std::string& key)
{
    auto it = _programs.find(key);
    if (it != _programs.end())
    {
        if (_destroy && it->second)
            _destroy(it->second);
        _programs.erase(it);
    }
}

void ShaderRegistry::Clear()
{
    if (_destroy)
    {
        for (auto& [key, handle] : _programs)
        {
            if (handle)
                _destroy(handle);
        }
    }
    _programs.clear();
}

} // namespace planetgen
