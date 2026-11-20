#include "AudioTrack.hpp"
#include <algorithm>
#include <cstring>

namespace Audiee {
namespace Timeline {

AudioTrack::AudioTrack(const std::string& trackId, const std::string& filePath, uint32_t sampleRate)
    : trackId_(trackId), filePath_(filePath), sampleRate_(sampleRate) {
    equalizer_.setup(static_cast<float>(sampleRate_));
    compressor_.setup(static_cast<float>(sampleRate_));
    reverb_.setup(static_cast<float>(sampleRate_));
    noiseGate_.setup(static_cast<float>(sampleRate_));
}

AudioTrack::~AudioTrack() {
    unload();
}

bool AudioTrack::load() {
    ma_decoder_config config = ma_decoder_config_init(ma_format_f32, 2, sampleRate_); // Stereo 44.1/48kHz
    ma_decoder decoder;

    ma_result res = ma_decoder_init_file(filePath_.c_str(), &config, &decoder);
    if (res != MA_SUCCESS) {
        return false;
    }

    channels_ = 2;
    ma_uint64 lengthInFrames = 0;
    ma_decoder_get_length_in_pcm_frames(&decoder, &lengthInFrames);
    totalFrames_ = static_cast<size_t>(lengthInFrames);
    durationSeconds_ = static_cast<double>(totalFrames_) / sampleRate_;

    pcmBuffer_.resize(totalFrames_ * channels_);
    ma_uint64 framesRead = 0;
    ma_decoder_read_pcm_frames(&decoder, pcmBuffer_.data(), totalFrames_, &framesRead);

    ma_decoder_uninit(&decoder);
    return true;
}

void AudioTrack::unload() {
    pcmBuffer_.clear();
    pcmBuffer_.shrink_to_fit();
    totalFrames_ = 0;
}

void AudioTrack::setTrim(double startSeconds, double endSeconds) {
    trimStartSec_ = std::max(0.0, startSeconds);
    trimEndSec_ = endSeconds;
}

void AudioTrack::setDenoise(float intensity) {
    noiseGate_.setIntensity(intensity);
}

void AudioTrack::setWarmth(float gainDb) {
    equalizer_.setWarmth(gainDb);
}

void AudioTrack::setAir(float gainDb) {
    equalizer_.setAir(gainDb);
}

void AudioTrack::setReverb(float mix, float roomSize) {
    reverb_.setWetMix(mix);
    reverb_.setRoomSize(roomSize);
}

void AudioTrack::setCompressor(float ratio, float thresholdDb) {
    compressor_.setRatio(ratio);
    compressor_.setThreshold(thresholdDb);
}

void AudioTrack::readSamples(int64_t timelineSample, float* outInterleavedStereo, size_t frameCount) {
    if (isMuted_ || pcmBuffer_.empty() || frameCount == 0) {
        return;
    }

    // Convert track timeOffsetMs to sample offset
    int64_t trackStartSample = (timeOffsetMs_ * sampleRate_) / 1000;
    int64_t trimStartSample = static_cast<int64_t>(trimStartSec_ * sampleRate_);
    int64_t trimEndSample = (trimEndSec_ > 0.0) 
        ? static_cast<int64_t>(trimEndSec_ * sampleRate_) 
        : static_cast<int64_t>(totalFrames_);

    // Check if timeline playback overlaps with this track
    int64_t localSampleStart = timelineSample - trackStartSample + trimStartSample;

    if (dspScratchBuffer_.size() < frameCount * 2) {
        dspScratchBuffer_.resize(frameCount * 2);
    }
    std::fill(dspScratchBuffer_.begin(), dspScratchBuffer_.begin() + frameCount * 2, 0.0f);

    bool hasAudio = false;

    for (size_t f = 0; f < frameCount; ++f) {
        int64_t curSample = localSampleStart + f;
        if (curSample >= trimStartSample && curSample < trimEndSample && curSample < static_cast<int64_t>(totalFrames_)) {
            dspScratchBuffer_[f * 2] = pcmBuffer_[curSample * 2];
            dspScratchBuffer_[f * 2 + 1] = pcmBuffer_[curSample * 2 + 1];
            hasAudio = true;
        }
    }

    if (!hasAudio) return;

    // Apply DSP Chain in sequence: NoiseGate -> Equalizer -> Compressor -> Reverb
    noiseGate_.processBlock(dspScratchBuffer_.data(), frameCount, 2);
    equalizer_.processBlock(dspScratchBuffer_.data(), frameCount, 2);
    compressor_.processBlock(dspScratchBuffer_.data(), frameCount, 2);
    reverb_.processInterleaved(dspScratchBuffer_.data(), frameCount, 2);

    // Apply Track Volume & Stereo Panning
    float leftGain = volume_ * std::min(1.0f, 1.0f - pan_);
    float rightGain = volume_ * std::min(1.0f, 1.0f + pan_);

    for (size_t f = 0; f < frameCount; ++f) {
        outInterleavedStereo[f * 2] += dspScratchBuffer_[f * 2] * leftGain;
        outInterleavedStereo[f * 2 + 1] += dspScratchBuffer_[f * 2 + 1] * rightGain;
    }
}

} // namespace Timeline
} // namespace Audiee
