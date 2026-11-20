#pragma once
#include <string>
#include <functional>
#include <vector>
#include "SpectralTransform.hpp"

namespace Audiee {
namespace AI {

using ProgressCallback = std::function<void(float progress01)>;

struct SeparationResult {
    bool success = false;
    std::string vocalPath;
    std::string instrumentalPath;
    double durationSeconds = 0.0;
    std::string errorMessage;
};

class StemSeparator {
public:
    StemSeparator(size_t fftSize = 2048, size_t hopSize = 512);

    /**
     * Separates any mixed audio file into two stems:
     * 1. Isolated Vocals (Speech / Singing)
     * 2. Clean Instrumental (Backing track / Karaoke / Drums / Bass)
     * 
     * @param inputFilePath Path to source WAV/MP3/FLAC
     * @param outputVocalPath Destination path for isolated vocal WAV
     * @param outputInstPath Destination path for instrumental WAV
     * @param progress Optional progress callback (0.0 to 1.0)
     */
    SeparationResult separate(
        const std::string& inputFilePath,
        const std::string& outputVocalPath,
        const std::string& outputInstPath,
        ProgressCallback progress = nullptr
    );

private:
    void applyVocalMasks(
        const std::vector<SpectrogramFrame>& inFrames,
        std::vector<SpectrogramFrame>& vocalFrames,
        std::vector<SpectrogramFrame>& instFrames,
        uint32_t sampleRate
    );

    SpectralTransform transform_;
};

} // namespace AI
} // namespace Audiee
