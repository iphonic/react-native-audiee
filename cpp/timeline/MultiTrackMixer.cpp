#include "MultiTrackMixer.hpp"
#include <algorithm>
#include <cmath>

namespace Audiee {
namespace Timeline {

MultiTrackMixer::MultiTrackMixer(uint32_t sampleRate) : sampleRate_(sampleRate) {}

MultiTrackMixer::~MultiTrackMixer() {
    clearTracks();
}

std::shared_ptr<AudioTrack> MultiTrackMixer::addTrack(const std::string& trackId, const std::string& filePath) {
    std::lock_guard<std::mutex> lock(trackMutex_);
    auto track = std::make_shared<AudioTrack>(trackId, filePath, sampleRate_);
    if (track->load()) {
        tracks_.push_back(track);
        return track;
    }
    return nullptr;
}

bool MultiTrackMixer::removeTrack(const std::string& trackId) {
    std::lock_guard<std::mutex> lock(trackMutex_);
    auto it = std::remove_if(tracks_.begin(), tracks_.end(), [&](const std::shared_ptr<AudioTrack>& t) {
        return t->getId() == trackId;
    });
    if (it != tracks_.end()) {
        tracks_.erase(it, tracks_.end());
        return true;
    }
    return false;
}

std::shared_ptr<AudioTrack> MultiTrackMixer::getTrack(const std::string& trackId) {
    std::lock_guard<std::mutex> lock(trackMutex_);
    for (const auto& t : tracks_) {
        if (t->getId() == trackId) return t;
    }
    return nullptr;
}

void MultiTrackMixer::clearTracks() {
    std::lock_guard<std::mutex> lock(trackMutex_);
    tracks_.clear();
}

void MultiTrackMixer::processPlayback(int64_t timelineSample, float* outStereoBuffer, size_t frameCount) {
    std::lock_guard<std::mutex> lock(trackMutex_);

    // Zero out the output buffer
    std::fill(outStereoBuffer, outStereoBuffer + (frameCount * 2), 0.0f);

    // Check if any track is soloed
    bool anySolo = false;
    for (const auto& t : tracks_) {
        if (t->isSolo()) {
            anySolo = true;
            break;
        }
    }

    // Mix in active tracks
    for (const auto& track : tracks_) {
        if (anySolo && !track->isSolo()) continue;
        if (track->isMuted()) continue;

        track->readSamples(timelineSample, outStereoBuffer, frameCount);
    }

    // Apply Master Limiter to avoid distortion
    applyMasterLimiter(outStereoBuffer, frameCount);
}

void MultiTrackMixer::applyMasterLimiter(float* stereoBuffer, size_t frameCount) {
    for (size_t i = 0; i < frameCount * 2; ++i) {
        float sample = stereoBuffer[i] * masterVolume_;
        // Transparent soft-knee brickwall limiter at -0.1 dBFS
        if (sample > 0.98f) {
            sample = 0.98f + (1.0f - 0.98f) * std::tanh((sample - 0.98f) / (1.0f - 0.98f));
        } else if (sample < -0.98f) {
            sample = -0.98f + (-1.0f - -0.98f) * std::tanh((sample - -0.98f) / (-1.0f - -0.98f));
        }
        stereoBuffer[i] = sample;
    }
}

double MultiTrackMixer::getTotalDurationSeconds() const {
    std::lock_guard<std::mutex> lock(trackMutex_);
    double maxDur = 0.0;
    for (const auto& t : tracks_) {
        double trackEnd = (t->getTimeOffsetMs() * 0.001) + t->getDurationSeconds();
        if (trackEnd > maxDur) maxDur = trackEnd;
    }
    return maxDur;
}

bool MultiTrackMixer::exportMix(const ExportOptions& options) {
    double totalSeconds = getTotalDurationSeconds();
    if (totalSeconds <= 0.0) return false;

    ma_encoder_config config = ma_encoder_config_init(
        ma_encoding_format_wav,
        ma_format_f32,
        2,
        sampleRate_
    );

    ma_encoder encoder;
    ma_result res = ma_encoder_init_file(options.outputPath.c_str(), &config, &encoder);
    if (res != MA_SUCCESS) {
        return false;
    }

    const size_t chunkSize = 2048;
    std::vector<float> buffer(chunkSize * 2);
    int64_t totalFrames = static_cast<int64_t>(totalSeconds * sampleRate_);
    int64_t currentFrame = 0;

    while (currentFrame < totalFrames) {
        size_t framesToRead = std::min<size_t>(chunkSize, totalFrames - currentFrame);
        processPlayback(currentFrame, buffer.data(), framesToRead);

        ma_encoder_write_pcm_frames(&encoder, buffer.data(), framesToRead, nullptr);
        currentFrame += framesToRead;
    }

    ma_encoder_uninit(&encoder);
    return true;
}

} // namespace Timeline
} // namespace Audiee
