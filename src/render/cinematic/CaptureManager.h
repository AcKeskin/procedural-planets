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
} // namespace CaptureDefaults

class CaptureManager
{
public:
    // Auto-named capture into the captures/ directory (F12 path).
    bool CaptureScreenshot(int width, int height);

    // Capture to an explicit path (headless --screenshot path). Creates parent dirs.
    bool CaptureScreenshotToFile(const std::string& path, int width, int height);

    const std::string& GetLastCapturePath() const { return _lastCapturePath; }

private:
    std::string GenerateFilename(const std::string& extension) const;
    bool EnsureCaptureDirectory() const;

    // Shared core: read the framebuffer, flip vertically, write a PNG to path.
    bool WritePng(const std::string& path, int width, int height);

    std::string _lastCapturePath;
    std::vector<uint8_t> _pixelBuffer;
};

} // namespace planets::render
