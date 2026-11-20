#define MINIAUDIO_IMPLEMENTATION
#include "AudieeEngine.hpp"
#include <cmath>
#include <algorithm>

namespace Audiee {

AudieeEngine::AudieeEngine(uint32_t sampleRate)
    : sampleRate_(sampleRate) {
    mixer_ = std::make_shared<Timeline::MultiTrackMixer>(sampleRate_);
}

AudieeEngine::~AudieeEngine() {
    shutdown();
}

bool AudieeEngine::initialize() {
    // 1. Setup Playback Device Config (Stereo, f32)
    ma_device_config playbackConfig = ma_device_config_init(ma_device_type_playback);
    playbackConfig.playback.format = ma_format_f32;
    playbackConfig.playback.channels = 2;
    playbackConfig.sampleRate = sampleRate_;
    playbackConfig.dataCallback = onPlaybackData;
    playbackConfig.pUserData = this;

    ma_result res = ma_device_init(nullptr, &playbackConfig, &playbackDevice_);
    if (res != MA_SUCCESS) {
        return false;
    }
    isPlaybackDeviceInit_ = true;

    // 2. Setup Capture/Recording Device Config (Mono/Stereo f32)
    ma_device_config captureConfig = ma_device_config_init(ma_device_type_capture);
    captureConfig.capture.format = ma_format_f32;
    captureConfig.capture.channels = 1;
    captureConfig.sampleRate = sampleRate_;
    captureConfig.dataCallback = onCaptureData;
    captureConfig.pUserData = this;

    res = ma_device_init(nullptr, &captureConfig, &captureDevice_);
    if (res == MA_SUCCESS) {
        isCaptureDeviceInit_ = true;
    }

    return true;
}

void AudieeEngine::shutdown() {
    stop();
    stopRecording();

    if (isPlaybackDeviceInit_) {
        ma_device_uninit(&playbackDevice_);
        isPlaybackDeviceInit_ = false;
    }

    if (isCaptureDeviceInit_) {
        ma_device_uninit(&captureDevice_);
        isCaptureDeviceInit_ = false;
    }
}

bool AudieeEngine::play() {
    if (!isPlaybackDeviceInit_) return false;
    if (ma_device_start(&playbackDevice_) == MA_SUCCESS) {
        isPlaying_ = true;
        return true;
    }
    return false;
}

bool AudieeEngine::pause() {
    if (!isPlaybackDeviceInit_) return false;
    if (ma_device_stop(&playbackDevice_) == MA_SUCCESS) {
        isPlaying_ = false;
        return true;
    }
    return false;
}

void AudieeEngine::stop() {
    pause();
    currentPlaybackSample_ = 0;
}

void AudieeEngine::seekTo(double seconds) {
    currentPlaybackSample_ = static_cast<int64_t>(std::max(0.0, seconds) * sampleRate_);
}

double AudieeEngine::getCurrentTimeSeconds() const {
    return static_cast<double>(currentPlaybackSample_) / sampleRate_;
}

bool AudieeEngine::startRecording(const std::string& destinationWavPath) {
    if (!isCaptureDeviceInit_) return false;
    if (isRecording_) stopRecording();

    ma_encoder_config config = ma_encoder_config_init(
        ma_encoding_format_wav,
        ma_format_f32,
        1, // Mono high-fidelity recording
        sampleRate_
    );

    ma_result res = ma_encoder_init_file(destinationWavPath.c_str(), &config, &recordingEncoder_);
    if (res != MA_SUCCESS) {
        return false;
    }
    isRecordingEncoderInit_ = true;

    if (ma_device_start(&captureDevice_) == MA_SUCCESS) {
        isRecording_ = true;
        return true;
    }

    ma_encoder_uninit(&recordingEncoder_);
    isRecordingEncoderInit_ = false;
    return false;
}

bool AudieeEngine::stopRecording() {
    if (!isRecording_) return true;

    ma_device_stop(&captureDevice_);
    isRecording_ = false;

    if (isRecordingEncoderInit_) {
        ma_encoder_uninit(&recordingEncoder_);
        isRecordingEncoderInit_ = false;
    }

    return true;
}

void AudieeEngine::onPlaybackData(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
    (void)pInput;
    auto* self = static_cast<AudieeEngine*>(pDevice->pUserData);
    if (!self || !self->isPlaying_) return;

    float* outStereo = static_cast<float*>(pOutput);
    self->mixer_->processPlayback(self->currentPlaybackSample_, outStereo, frameCount);
    self->currentPlaybackSample_ += frameCount;
}

void AudieeEngine::onCaptureData(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
    (void)pOutput;
    auto* self = static_cast<AudieeEngine*>(pDevice->pUserData);
    if (!self || !self->isRecording_ || !pInput) return;

    const float* inMono = static_cast<const float*>(pInput);

    // 1. Write frames to WAV encoder
    if (self->isRecordingEncoderInit_) {
        ma_encoder_write_pcm_frames(&self->recordingEncoder_, inMono, frameCount, nullptr);
    }

    // 2. Compute RMS and Peak for live VU Metering callback
    if (self->meteringCallback_) {
        float sumSquare = 0.0f;
        float peak = 0.0f;

        for (ma_uint32 i = 0; i < frameCount; ++i) {
            float s = std::abs(inMono[i]);
            sumSquare += s * s;
            if (s > peak) peak = s;
        }

        float rms = std::sqrt(sumSquare / frameCount);
        float rmsDb = (rms > 0.00001f) ? (20.0f * std::log10(rms)) : -100.0f;
        float peakDb = (peak > 0.00001f) ? (20.0f * std::log10(peak)) : -100.0f;

        self->meteringCallback_(rmsDb, peakDb);
    }
}

} // namespace Audiee
