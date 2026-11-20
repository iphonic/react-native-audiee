#pragma once
#include <vector>
#include <algorithm>
#include <cmath>

namespace Audiee {
namespace DSP {

class CombFilter {
public:
    void setBufferSize(size_t size) {
        buffer_.assign(size, 0.0f);
        bufIdx_ = 0;
        filterStore_ = 0.0f;
    }

    void setDamp(float damp) {
        damp1_ = damp;
        damp2_ = 1.0f - damp;
    }

    void setFeedback(float feedback) {
        feedback_ = feedback;
    }

    inline float process(float input) {
        float output = buffer_[bufIdx_];
        filterStore_ = (output * damp2_) + (filterStore_ * damp1_);
        buffer_[bufIdx_] = input + (filterStore_ * feedback_);
        if (++bufIdx_ >= buffer_.size()) bufIdx_ = 0;
        return output;
    }

    void mute() {
        std::fill(buffer_.begin(), buffer_.end(), 0.0f);
        filterStore_ = 0.0f;
    }

private:
    std::vector<float> buffer_;
    size_t bufIdx_ = 0;
    float feedback_ = 0.5f;
    float filterStore_ = 0.0f;
    float damp1_ = 0.2f;
    float damp2_ = 0.8f;
};

class AllpassFilter {
public:
    void setBufferSize(size_t size) {
        buffer_.assign(size, 0.0f);
        bufIdx_ = 0;
    }

    void setFeedback(float feedback) {
        feedback_ = feedback;
    }

    inline float process(float input) {
        float bufOut = buffer_[bufIdx_];
        float output = -input + bufOut;
        buffer_[bufIdx_] = input + (bufOut * feedback_);
        if (++bufIdx_ >= buffer_.size()) bufIdx_ = 0;
        return output;
    }

    void mute() {
        std::fill(buffer_.begin(), buffer_.end(), 0.0f);
    }

private:
    std::vector<float> buffer_;
    size_t bufIdx_ = 0;
    float feedback_ = 0.5f;
};

/**
 * Freeverb: High quality, low CPU stereo algorithmic reverb
 */
class Freeverb {
public:
    Freeverb() {
        setup(44100.0f);
    }

    void setup(float sampleRate) {
        const float scale = sampleRate / 44100.0f;
        const int combTunings[8] = { 1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617 };
        const int allpassTunings[4] = { 556, 441, 341, 225 };
        const int stereoSpread = 23;

        for (int i = 0; i < 8; ++i) {
            combL_[i].setBufferSize(static_cast<size_t>(combTunings[i] * scale));
            combR_[i].setBufferSize(static_cast<size_t>((combTunings[i] + stereoSpread) * scale));
        }

        for (int i = 0; i < 4; ++i) {
            allpassL_[i].setBufferSize(static_cast<size_t>(allpassTunings[i] * scale));
            allpassR_[i].setBufferSize(static_cast<size_t>((allpassTunings[i] + stereoSpread) * scale));
            allpassL_[i].setFeedback(0.5f);
            allpassR_[i].setFeedback(0.5f);
        }

        updateParams();
    }

    void setRoomSize(float value) {
        roomSize_ = std::clamp(value, 0.0f, 1.0f);
        updateParams();
    }

    void setDamping(float value) {
        damping_ = std::clamp(value, 0.0f, 1.0f);
        updateParams();
    }

    void setWetMix(float value) {
        wetMix_ = std::clamp(value, 0.0f, 1.0f);
    }

    void processStereo(float* inL, float* inR, float* outL, float* outR, size_t numSamples) {
        if (wetMix_ <= 0.001f) {
            for (size_t i = 0; i < numSamples; ++i) {
                outL[i] = inL[i];
                outR[i] = inR[i];
            }
            return;
        }

        const float dryMix = 1.0f - wetMix_ * 0.5f;
        const float wet1 = wetMix_ * (width_ * 0.5f + 0.5f);
        const float wet2 = wetMix_ * ((1.0f - width_) * 0.5f);

        for (size_t i = 0; i < numSamples; ++i) {
            float monoIn = (inL[i] + inR[i]) * 0.015f; // input gain scale
            float outL_reverb = 0.0f;
            float outR_reverb = 0.0f;

            // 8 parallel comb filters
            for (int c = 0; c < 8; ++c) {
                outL_reverb += combL_[c].process(monoIn);
                outR_reverb += combR_[c].process(monoIn);
            }

            // 4 series allpass filters
            for (int a = 0; a < 4; ++a) {
                outL_reverb = allpassL_[a].process(outL_reverb);
                outR_reverb = allpassR_[a].process(outR_reverb);
            }

            outL[i] = inL[i] * dryMix + outL_reverb * wet1 + outR_reverb * wet2;
            outR[i] = inR[i] * dryMix + outR_reverb * wet1 + outL_reverb * wet2;
        }
    }

    void processInterleaved(float* buffer, size_t frameCount, size_t channels) {
        if (wetMix_ <= 0.001f) return;

        if (channels == 1) {
            for (size_t i = 0; i < frameCount; ++i) {
                float in = buffer[i];
                float outL = 0.0f, outR = 0.0f;
                processStereo(&in, &in, &outL, &outR, 1);
                buffer[i] = (outL + outR) * 0.5f;
            }
        } else if (channels == 2) {
            for (size_t i = 0; i < frameCount; ++i) {
                float inL = buffer[i * 2];
                float inR = buffer[i * 2 + 1];
                float outL = 0.0f, outR = 0.0f;
                processStereo(&inL, &inR, &outL, &outR, 1);
                buffer[i * 2] = outL;
                buffer[i * 2 + 1] = outR;
            }
        }
    }

private:
    void updateParams() {
        const float feedback = roomSize_ * 0.28f + 0.7f;
        for (int i = 0; i < 8; ++i) {
            combL_[i].setFeedback(feedback);
            combR_[i].setFeedback(feedback);
            combL_[i].setDamp(damping_ * 0.4f);
            combR_[i].setDamp(damping_ * 0.4f);
        }
    }

    float roomSize_ = 0.5f;
    float damping_ = 0.2f;
    float wetMix_ = 0.0f;
    float width_ = 1.0f;

    CombFilter combL_[8];
    CombFilter combR_[8];
    AllpassFilter allpassL_[4];
    AllpassFilter allpassR_[4];
};

} // namespace DSP
} // namespace Audiee
