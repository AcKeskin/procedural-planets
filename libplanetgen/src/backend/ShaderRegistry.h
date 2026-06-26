#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <functional>

namespace planetgen
{

// Stable ids for the four built-in compute programs
enum class BuiltinProgram : int
{
    Height = 0,
    ShadingEarth = 1,
    ShadingGeneric = 2,
    Erosion = 3,
};

// Converts a BuiltinProgram id to its canonical string key
inline std::string BuiltinKey(BuiltinProgram id)
{
    switch (id)
    {
    case BuiltinProgram::Height:         return "__builtin_height";
    case BuiltinProgram::ShadingEarth:   return "__builtin_shading_earth";
    case BuiltinProgram::ShadingGeneric: return "__builtin_shading_generic";
    case BuiltinProgram::Erosion:        return "__builtin_erosion";
    }
    return "";
}

// Maps string keys to compiled GL program handles.
// Programs are compiled once and cached; never re-compiled unless Unregister is called.
// Compile is delegated through a caller-supplied functor so the registry stays GL-agnostic.
class ShaderRegistry
{
public:
    using CompileFn = std::function<uint32_t(const std::string& source)>;
    using DestroyFn = std::function<void(uint32_t program)>;

    ShaderRegistry() = default;
    ~ShaderRegistry();

    ShaderRegistry(const ShaderRegistry&) = delete;
    ShaderRegistry& operator=(const ShaderRegistry&) = delete;

    void SetCompiler(CompileFn compile, DestroyFn destroy);

    // Register a source under a key. Compiles immediately; returns false if compile fails.
    bool RegisterProgram(const std::string& key, const std::string& source);

    // Convenience overload for built-in ids
    bool RegisterProgram(BuiltinProgram id, const std::string& source);

    // Returns the compiled program handle, or 0 if not found
    uint32_t Resolve(const std::string& key) const;
    uint32_t Resolve(BuiltinProgram id) const;

    // Destroy one registered program and remove it from the map
    void Unregister(const std::string& key);

    // Destroy all registered programs
    void Clear();

private:
    CompileFn _compile;
    DestroyFn _destroy;
    std::unordered_map<std::string, uint32_t> _programs;
};

} // namespace planetgen
