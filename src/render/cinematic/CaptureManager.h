#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace planets::render
{

namespace CaptureDefaults
{
    inline constexpr const char* Directory = "captures";
    inline constexpr const char* FilenamePrefix = "planet_";
    inline constexpr const char* TimestampFormat = "%Y-%m-%d_%H%M%S";
    inline constexpr const char* PngExtension = ".png";
    inline constexpr int RgbChannels = 3;
}

class CaptureManager
{
public:
    bool CaptureScreenshot(int width, int height);

    const std::string& GetLastCapturePath() const { return _lastCapturePath; }

private:
    std::string GenerateFilename(const std::string& extension) const;
    bool EnsureCaptureDirectory() const;

    std::string _lastCapturePath;
    std::vector<uint8_t> _pixelBuffer;
};

} // namespace planets::render
