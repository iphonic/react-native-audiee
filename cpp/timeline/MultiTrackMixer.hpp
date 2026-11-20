#pragma once
#include <vector>
#include <memory>
#include <string>
#include <mutex>
#include "AudioTrack.hpp"
#include "../alignment/CrossCorrelation.hpp"

namespace Audiee {
namespace Timeline {

struct ExportOptions {
    std::string outputPath;
    float normalizeTargetLufs = -14.0f; // -14 LUFS standard, or 0.0 for raw
    uint32_t sampleRate = 44100;
};

class MultiTrackMixer {
public:
    MultiTrackMixer(uint32_t sampleRate = 44100);
    ~MultiTrackMixer();

    std::shared_ptr<AudioTrack> addTrack(const std::string& trackId, const std::string& filePath);
    bool removeTrack(const std::string& trackId);
    std::shared_ptr<AudioTrack> getTrack(const std::string& trackId);
    void clearTracks();

    // Timeline Playback Processing (called by real-time miniaudio playback callback)
    void processPlayback(int64_t timelineSample, float* outStereoBuffer, size_t frameCount);

    // Smart Auto-Alignment (Pro Feature)
    Alignment::AlignmentResult autoAlignTracks(const std::string& refTrackId, const std::string& targetTrackId);

    // Offline File Export (WAV / MP3)
    bool exportMix(const ExportOptions& options);

    double getTotalDurationSeconds() const;
    uint32_t getSampleRate() const { return sampleRate_; }

private:
    uint32_t sampleRate_ = 44100;
    std::vector<std::shared_ptr<AudioTrack>> tracks_;
    mutable std::mutex trackMutex_;

    // Master Peak Limiter
    float masterVolume_ = 1.0f;
    void applyMasterLimiter(float* stereoBuffer, size_t frameCount);
};

} // namespace Timeline
} // namespace Audiee
