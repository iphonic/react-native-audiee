#pragma once
#include <string>
#include <memory>
#include <functional>
#include "miniaudio.h"
#include "timeline/MultiTrackMixer.hpp"
#include "waveform/PeakExtractor.hpp"
#include "ai/StemSeparator.hpp"

namespace Audiee {

using MeteringCallback = std::function<void(float rmsDb, float peakDb)>;

class AudieeEngine {
public:
    AudieeEngine(uint32_t sampleRate = 44100);
    ~AudieeEngine();

    bool initialize();
    void shutdown();

    // Playback Engine
    bool play();
    bool pause();
    void stop();
    void seekTo(double seconds);
    double getCurrentTimeSeconds() const;
    bool isPlaying() const { return isPlaying_; }

    // Recording Engine (Dolby On Style)
    bool startRecording(const std::string& destinationWavPath);
    bool stopRecording();
    bool isRecording() const { return isRecording_; }
    void setMeteringCallback(MeteringCallback callback) { meteringCallback_ = callback; }

    // Timeline Mixer Access
    std::shared_ptr<Timeline::MultiTrackMixer> getMixer() { return mixer_; }

    // Static Utility Methods
    static Waveform::WaveformData extractWaveform(const std::string& filePath, uint32_t pps = 50) {
        return Waveform::PeakExtractor::extractPeaks(filePath, pps);
    }

    // AI Stem Separator (Vocals vs Instrumental Isolation)
    static AI::SeparationResult separateStems(
        const std::string& inputFilePath,
        const std::string& outputVocalPath,
        const std::string& outputInstPath,
        AI::ProgressCallback progress = nullptr
    ) {
        AI::StemSeparator separator;
        return separator.separate(inputFilePath, outputVocalPath, outputInstPath, progress);
    }

private:
    static void onPlaybackData(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount);
    static void onCaptureData(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount);

    uint32_t sampleRate_ = 44100;
    std::shared_ptr<Timeline::MultiTrackMixer> mixer_;

    ma_device playbackDevice_;
    ma_device captureDevice_;
    bool isPlaybackDeviceInit_ = false;
    bool isCaptureDeviceInit_ = false;

    bool isPlaying_ = false;
    bool isRecording_ = false;
    int64_t currentPlaybackSample_ = 0;

    ma_encoder recordingEncoder_;
    bool isRecordingEncoderInit_ = false;
    MeteringCallback meteringCallback_;
};

} // namespace Audiee
