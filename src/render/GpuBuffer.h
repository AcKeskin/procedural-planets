#pragma once

#include <GL/gl3w.h>
#include <vector>
#include <cstddef>

namespace planets::render
{

// GPU storage buffer (SSBO) for compute shader data transfer
template <typename T> class GpuBuffer
{
public:
    GpuBuffer();
    ~GpuBuffer();

    GpuBuffer(const GpuBuffer&) = delete;
    GpuBuffer& operator=(const GpuBuffer&) = delete;

    GpuBuffer(GpuBuffer&& other) noexcept;
    GpuBuffer& operator=(GpuBuffer&& other) noexcept;

    // Upload data to GPU
    void Upload(const std::vector<T>& data);
    void Upload(const T* data, size_t count);

    // Allocate buffer without initial data
    void Allocate(size_t count);

    // Download data from GPU
    void Download(std::vector<T>& data) const;
    void Download(T* data, size_t count) const;

    // Bind buffer to shader storage binding point
    void Bind(unsigned int bindingPoint) const;

    // Accessors
    unsigned int GetHandle() const { return _buffer; }
    size_t GetCount() const { return _count; }
    size_t GetSizeBytes() const { return _count * sizeof(T); }
    bool IsValid() const { return _buffer != 0; }

private:
    unsigned int _buffer;
    size_t _count;
};

// Template implementation
template <typename T>
GpuBuffer<T>::GpuBuffer()
    : _buffer(0)
    , _count(0)
{
}

template <typename T> GpuBuffer<T>::~GpuBuffer()
{
    if (_buffer)
    {
        glDeleteBuffers(1, &_buffer);
    }
}

template <typename T>
GpuBuffer<T>::GpuBuffer(GpuBuffer&& other) noexcept
    : _buffer(other._buffer)
    , _count(other._count)
{
    other._buffer = 0;
    other._count = 0;
}

template <typename T> GpuBuffer<T>& GpuBuffer<T>::operator=(GpuBuffer&& other) noexcept
{
    if (this != &other)
    {
        if (_buffer)
        {
            glDeleteBuffers(1, &_buffer);
        }
        _buffer = other._buffer;
        _count = other._count;
        other._buffer = 0;
        other._count = 0;
    }
    return *this;
}

template <typename T> void GpuBuffer<T>::Upload(const std::vector<T>& data)
{
    Upload(data.data(), data.size());
}

template <typename T> void GpuBuffer<T>::Upload(const T* data, size_t count)
{
    if (!_buffer)
    {
        glGenBuffers(1, &_buffer);
    }

    _count = count;
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, _buffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, count * sizeof(T), data, GL_DYNAMIC_COPY);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

template <typename T> void GpuBuffer<T>::Allocate(size_t count)
{
    if (!_buffer)
    {
        glGenBuffers(1, &_buffer);
    }

    _count = count;
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, _buffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, count * sizeof(T), nullptr, GL_DYNAMIC_COPY);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

template <typename T> void GpuBuffer<T>::Download(std::vector<T>& data) const
{
    data.resize(_count);
    Download(data.data(), _count);
}

template <typename T> void GpuBuffer<T>::Download(T* data, size_t count) const
{
    if (!_buffer || count == 0)
    {
        return;
    }

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, _buffer);
    glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, count * sizeof(T), data);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

template <typename T> void GpuBuffer<T>::Bind(unsigned int bindingPoint) const
{
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, bindingPoint, _buffer);
}

} // namespace planets::render
