#pragma once
#include <string>
#include <vector>
#include <memory>
#include "../miniaudio.h"
#include "../dsp/BiquadFilter.hpp"
#include "../dsp/Compressor.hpp"
#include "../dsp/Freeverb.hpp"
#include "../dsp/NoiseGate.hpp"

namespace Audiee {
namespace Timeline {

class AudioTrack {
public:
    AudioTrack(const std::string& trackId, const std::string& filePath, uint32_t sampleRate = 44100);
    ~AudioTrack();

    bool load();
    void unload();

    // Timeline Controls
    std::string getId() const { return trackId_; }
    std::string getFilePath() const { return filePath_; }

    void setTimeOffsetMs(int64_t offsetMs) { timeOffsetMs_ = offsetMs; }
    int64_t getTimeOffsetMs() const { return timeOffsetMs_; }

    void setTrim(double startSeconds, double endSeconds);
    double getDurationSeconds() const { return durationSeconds_; }

    void setVolume(float volume) { volume_ = volume; }
    float getVolume() const { return volume_; }

    void setPan(float pan) { pan_ = pan; } // -1.0 (Left) to +1.0 (Right)
    float getPan() const { return pan_; }

    void setMute(bool mute) { isMuted_ = mute; }
    bool isMuted() const { return isMuted_; }

    void setSolo(bool solo) { isSolo_ = solo; }
    bool isSolo() const { return isSolo_; }

    // Real-Time DSP Controls
    void setDenoise(float intensity);
    void setWarmth(float gainDb);
    void setAir(float gainDb);
    void setReverb(float mix, float roomSize = 0.5f);
    void setCompressor(float ratio, float thresholdDb = -16.0f);

    // Audio Rendering: reads samples corresponding to timeline position 'timelineSample'
    void readSamples(int64_t timelineSample, float* outInterleavedStereo, size_t frameCount);

    // Get raw decoded PCM buffer (for analysis, visualizers, or custom extensions)
    const std::vector<float>& getRawPCM() const { return pcmBuffer_; }
    uint32_t getSampleRate() const { return sampleRate_; }

private:
    std::string trackId_;
    std::string filePath_;
    uint32_t sampleRate_ = 44100;
    double durationSeconds_ = 0.0;

    int64_t timeOffsetMs_ = 0;
    double trimStartSec_ = 0.0;
    double trimEndSec_ = -1.0; // -1 means until end

    float volume_ = 1.0f;
    float pan_ = 0.0f;
    bool isMuted_ = false;
    bool isSolo_ = false;

    // Loaded Audio Buffer (Mono or Stereo 32-bit float)
    std::vector<float> pcmBuffer_;
    uint32_t channels_ = 2;
    size_t totalFrames_ = 0;

    // Track DSP Chain
    DSP::Equalizer3Band equalizer_;
    DSP::Compressor compressor_;
    DSP::Freeverb reverb_;
    DSP::NoiseGate noiseGate_;

    std::vector<float> dspScratchBuffer_;
};

} // namespace Timeline
} // namespace Audiee
