#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <functional>

namespace planetgen
{

// Stable ids for built-in compute programs
enum class BuiltinProgram : int
{
    Height = 0,
    ShadingEarth = 1,
    ShadingGeneric = 2,
    Erosion = 3,
    ContinentGrowth = 4,
    MacroPlates = 5,
    MacroErosionRainfall = 6,
    MacroErosionFlow = 7,
    MacroErosionSediment = 8,
    MacroDrainageFlux = 9,
    MacroFill = 10,
    MacroRivers = 11,
    MacroClimate = 12,
    HeightMacro = 13,
    ShadingEarthMacro = 14,
    HeightStyle = 15,
    HeightMacroStyle = 16,
};

// Converts a BuiltinProgram id to its canonical string key
inline std::string BuiltinKey(BuiltinProgram id)
{
    switch (id)
    {
    case BuiltinProgram::Height:          return "__builtin_height";
    case BuiltinProgram::ShadingEarth:    return "__builtin_shading_earth";
    case BuiltinProgram::ShadingGeneric:  return "__builtin_shading_generic";
    case BuiltinProgram::Erosion:         return "__builtin_erosion";
    case BuiltinProgram::ContinentGrowth: return "__builtin_continent_growth";
    case BuiltinProgram::MacroPlates:     return "__builtin_macro_plates";
    case BuiltinProgram::MacroErosionRainfall: return "__builtin_macro_erosion_rainfall";
    case BuiltinProgram::MacroErosionFlow:     return "__builtin_macro_erosion_flow";
    case BuiltinProgram::MacroErosionSediment: return "__builtin_macro_erosion_sediment";
    case BuiltinProgram::MacroDrainageFlux:    return "__builtin_macro_drainage_flux";
    case BuiltinProgram::MacroFill:            return "__builtin_macro_fill";
    case BuiltinProgram::MacroRivers:          return "__builtin_macro_rivers";
    case BuiltinProgram::MacroClimate:         return "__builtin_macro_climate";
    case BuiltinProgram::HeightMacro:          return "__builtin_height_macro";
    case BuiltinProgram::ShadingEarthMacro:    return "__builtin_shading_earth_macro";
    case BuiltinProgram::HeightStyle:          return "__builtin_height_style";
    case BuiltinProgram::HeightMacroStyle:     return "__builtin_height_macro_style";
    }
    return "";
}

// Returns `source` with `#define <name> 1` inserted after its #version line, for
// compiling a feature variant of a built-in shader.
std::string WithDefine(const std::string& source, const std::string& name);

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
