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
        << std::put_time(&localTime, CaptureDefaults::TimestampFormat)
        << extension;

    return oss.str();
}

bool CaptureManager::CaptureScreenshot(int width, int height)
{
    if (!EnsureCaptureDirectory())
        return false;

    _pixelBuffer.resize(width * height * CaptureDefaults::RgbChannels);
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

    _lastCapturePath = GenerateFilename(CaptureDefaults::PngExtension);

    const int stride = width * CaptureDefaults::RgbChannels;
    const int success = stbi_write_png(_lastCapturePath.c_str(), width, height,
        CaptureDefaults::RgbChannels, _pixelBuffer.data(), stride);

    if (success)
    {
        std::cout << "[Capture] Screenshot saved: " << _lastCapturePath << std::endl;
        return true;
    }

    std::cerr << "[Capture] Failed to write PNG: " << _lastCapturePath << std::endl;
    return false;
}

} // namespace planets::render
