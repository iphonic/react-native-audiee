#pragma once
#include <cmath>
#include <algorithm>

namespace Audiee {
namespace DSP {

/**
 * Dynamic Range Compressor & Soft-Knee Peak Limiter
 * Automatically balances loud and quiet vocal sections without harsh pumping.
 */
class Compressor {
public:
    void setup(float sampleRate) {
        sampleRate_ = sampleRate;
        updateTimeConstants();
    }

    void setThreshold(float thresholdDb) {
        thresholdDb_ = thresholdDb;
    }

    void setRatio(float ratio) {
        ratio_ = std::max(1.0f, ratio);
    }

    void setAttack(float attackMs) {
        attackMs_ = std::max(0.1f, attackMs);
        updateTimeConstants();
    }

    void setRelease(float releaseMs) {
        releaseMs_ = std::max(5.0f, releaseMs);
        updateTimeConstants();
    }

    void setMakeupGain(float gainDb) {
        makeupGain_ = std::pow(10.0f, gainDb / 20.0f);
    }

    void processBlock(float* buffer, size_t frameCount, size_t channels) {
        for (size_t f = 0; f < frameCount; ++f) {
            // Find max peak across channels for sidechain detection
            float maxAbs = 0.0f;
            for (size_t ch = 0; ch < channels; ++ch) {
                maxAbs = std::max(maxAbs, std::abs(buffer[f * channels + ch]));
            }

            // Envelope detector
            if (maxAbs > envelope_) {
                envelope_ = attackCoeff_ * (envelope_ - maxAbs) + maxAbs;
            } else {
                envelope_ = releaseCoeff_ * (envelope_ - maxAbs) + maxAbs;
            }

            // Compute gain reduction in dB
            float envDb = (envelope_ > 0.00001f) ? (20.0f * std::log10(envelope_)) : -100.0f;
            float gainReductionDb = 0.0f;

            if (envDb > thresholdDb_) {
                gainReductionDb = (thresholdDb_ - envDb) * (1.0f - 1.0f / ratio_);
            }

            float gainLinear = std::pow(10.0f, gainReductionDb / 20.0f) * makeupGain_;

            // Apply gain and soft-knee peak limiting
            for (size_t ch = 0; ch < channels; ++ch) {
                float sample = buffer[f * channels + ch] * gainLinear;
                
                // Soft-clipping peak limiter at -0.1 dBFS to guarantee zero harsh digital clipping
                if (sample > 0.98f) {
                    sample = 0.98f + (1.0f - 0.98f) * std::tanh((sample - 0.98f) / (1.0f - 0.98f));
                } else if (sample < -0.98f) {
                    sample = -0.98f + (-1.0f - -0.98f) * std::tanh((sample - -0.98f) / (-1.0f - -0.98f));
                }

                buffer[f * channels + ch] = sample;
            }
        }
    }

private:
    void updateTimeConstants() {
        attackCoeff_ = std::exp(-1.0f / (sampleRate_ * (attackMs_ * 0.001f)));
        releaseCoeff_ = std::exp(-1.0f / (sampleRate_ * (releaseMs_ * 0.001f)));
    }

    float sampleRate_ = 44100.0f;
    float thresholdDb_ = -16.0f;
    float ratio_ = 4.0f;
    float attackMs_ = 10.0f;
    float releaseMs_ = 100.0f;
    float makeupGain_ = 1.0f;

    float envelope_ = 0.0f;
    float attackCoeff_ = 0.0f;
    float releaseCoeff_ = 0.0f;
};

} // namespace DSP
} // namespace Audiee
