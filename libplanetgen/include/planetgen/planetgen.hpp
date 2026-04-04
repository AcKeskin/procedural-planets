// C++ RAII convenience wrappers over the libplanetgen C API
// Header-only — include after planetgen.h

#ifndef PLANETGEN_HPP
#define PLANETGEN_HPP

#include "planetgen.h"
#include <memory>
#include <string>
#include <vector>
#include <stdexcept>

namespace pg
{

// RAII wrapper for PgResult
class Result
{
public:
    Result() : _handle(nullptr) {}
    explicit Result(PgResult handle) : _handle(handle) {}
    ~Result() { if (_handle) pg_result_destroy(_handle); }

    Result(const Result&) = delete;
    Result& operator=(const Result&) = delete;

    Result(Result&& other) noexcept : _handle(other._handle) { other._handle = nullptr; }
    Result& operator=(Result&& other) noexcept
    {
        if (this != &other)
        {
            if (_handle) pg_result_destroy(_handle);
            _handle = other._handle;
            other._handle = nullptr;
        }
        return *this;
    }

    const float* Heights() const { return pg_result_get_heights(_handle); }
    const float* Normals() const { return pg_result_get_normals(_handle); }
    const float* Shading() const { return pg_result_get_shading(_handle); }
    uint32_t Count() const { return pg_result_get_count(_handle); }
    uint32_t GpuBuffer() const { return pg_result_get_gpu_buffer(_handle); }

    bool IsValid() const { return _handle != nullptr; }
    PgResult Handle() const { return _handle; }

private:
    PgResult _handle;
};

// RAII wrapper for PgBody
class Body
{
public:
    Body() : _handle(nullptr) {}
    explicit Body(PgBody handle) : _handle(handle) {}
    ~Body() { if (_handle) pg_body_destroy(_handle); }

    Body(const Body&) = delete;
    Body& operator=(const Body&) = delete;

    Body(Body&& other) noexcept : _handle(other._handle) { other._handle = nullptr; }
    Body& operator=(Body&& other) noexcept
    {
        if (this != &other)
        {
            if (_handle) pg_body_destroy(_handle);
            _handle = other._handle;
            other._handle = nullptr;
        }
        return *this;
    }

    Result GenerateHeights(const float* vertices, uint32_t vertexCount, uint32_t seed)
    {
        return Result(pg_generate_heights(_handle, vertices, vertexCount, seed));
    }

    Result GenerateShading(const float* vertices, const float* heights,
                           uint32_t vertexCount, uint32_t seed, const PgShadingDesc& desc)
    {
        return Result(pg_generate_shading(_handle, vertices, heights, vertexCount, seed, &desc));
    }

    Result GenerateErosion(const float* heights, uint32_t vertexCount,
                           const PgErosionDesc& desc, uint32_t seed)
    {
        return Result(pg_generate_erosion(_handle, heights, vertexCount, &desc, seed));
    }

    bool IsValid() const { return _handle != nullptr; }
    PgBody Handle() const { return _handle; }

private:
    PgBody _handle;
};

// RAII wrapper for PgContext
class Context
{
public:
    explicit Context(bool useExistingContext = false)
    {
        PgContextDesc desc{};
        desc.use_existing_context = useExistingContext ? 1 : 0;
        _handle = pg_context_create(&desc);
        if (!_handle)
            throw std::runtime_error("Failed to create planetgen context");
    }

    ~Context() { if (_handle) pg_context_destroy(_handle); }

    Context(const Context&) = delete;
    Context& operator=(const Context&) = delete;

    Context(Context&& other) noexcept : _handle(other._handle) { other._handle = nullptr; }
    Context& operator=(Context&& other) noexcept
    {
        if (this != &other)
        {
            if (_handle) pg_context_destroy(_handle);
            _handle = other._handle;
            other._handle = nullptr;
        }
        return *this;
    }

    Body CreateBody(const PgBodyDesc& desc)
    {
        return Body(pg_body_create(_handle, &desc));
    }

    Body CreateBodyFromJson(const std::string& jsonPath)
    {
        return Body(pg_body_create_from_json(_handle, jsonPath.c_str()));
    }

    PgError GetError() const { return pg_context_get_error(_handle); }
    PgContext Handle() const { return _handle; }

private:
    PgContext _handle;
};

} // namespace pg

#endif // PLANETGEN_HPP
