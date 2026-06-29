#include "CaptureManager.h"
#include <GL/gl3w.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>
#include <filesystem>
#include <chrono>
#include <ctime>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>

namespace planets::render
{

bool CaptureManager::EnsureCaptureDirectory() const
{
    try
    {
        std::filesystem::create_directories(CaptureDefaults::Directory);
        return true;
    }
    catch (const std::filesystem::filesystem_error& e)
    {
        std::cerr << "[Capture] Failed to create captures directory: " << e.what() << std::endl;
        return false;
    }
}

std::string CaptureManager::GenerateFilename(const std::string& extension) const
{
    auto now = std::chrono::system_clock::now();
    auto timeT = std::chrono::system_clock::to_time_t(now);

    std::tm localTime;
#ifdef _WIN32
    localtime_s(&localTime, &timeT);
#else
    localtime_r(&timeT, &localTime);
#endif

    std::ostringstream oss;
    oss << CaptureDefaults::Directory << "/" << CaptureDefaults::FilenamePrefix
        << std::put_time(&localTime, CaptureDefaults::TimestampFormat) << extension;

    return oss.str();
}

bool CaptureManager::WritePng(const std::string& path, int width, int height)
{
    _pixelBuffer.resize(static_cast<size_t>(width) * height * CaptureDefaults::RgbChannels);
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, _pixelBuffer.data());

    // Flip vertically (OpenGL origin is bottom-left, PNG expects top-left)
    const int rowSize = width * CaptureDefaults::RgbChannels;
    std::vector<uint8_t> rowBuffer(rowSize);

    for (int y = 0; y < height / 2; ++y)
    {
        uint8_t* topRow = _pixelBuffer.data() + y * rowSize;
        uint8_t* bottomRow = _pixelBuffer.data() + (height - 1 - y) * rowSize;

        std::copy(topRow, topRow + rowSize, rowBuffer.data());
        std::copy(bottomRow, bottomRow + rowSize, topRow);
        std::copy(rowBuffer.data(), rowBuffer.data() + rowSize, bottomRow);
    }

    const int stride = width * CaptureDefaults::RgbChannels;
    const int success =
        stbi_write_png(path.c_str(), width, height, CaptureDefaults::RgbChannels, _pixelBuffer.data(), stride);

    if (success)
    {
        _lastCapturePath = path;
        std::cout << "[Capture] Screenshot saved: " << path << std::endl;
        return true;
    }

    std::cerr << "[Capture] Failed to write PNG: " << path << std::endl;
    return false;
}

bool CaptureManager::CaptureScreenshot(int width, int height)
{
    if (!EnsureCaptureDirectory())
        return false;

    return WritePng(GenerateFilename(CaptureDefaults::PngExtension), width, height);
}

bool CaptureManager::CaptureScreenshotToFile(const std::string& path, int width, int height)
{
    // Headless path: caller gives an explicit (usually absolute) path; ensure its parent exists.
    try
    {
        std::filesystem::path p(path);
        if (p.has_parent_path())
            std::filesystem::create_directories(p.parent_path());
    }
    catch (const std::filesystem::filesystem_error& e)
    {
        std::cerr << "[Capture] Failed to create output directory: " << e.what() << std::endl;
        return false;
    }

    return WritePng(path, width, height);
}

} // namespace planets::render
