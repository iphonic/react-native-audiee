#pragma once
#include <cmath>
#include <algorithm>

namespace Audiee {
namespace DSP {

/**
 * Noise Gate / Room Denoise Filter
 * Smooth downward expander that eliminates background room hiss and fan noise.
 */
class NoiseGate {
public:
    void setup(float sampleRate) {
        sampleRate_ = sampleRate;
        updateTimeConstants();
    }

    void setIntensity(float intensity01) {
        // Maps 0.0 -> 1.0 to threshold from -65 dBFS (gentle) to -28 dBFS (aggressive)
        float clamped = std::clamp(intensity01, 0.0f, 1.0f);
        thresholdDb_ = -65.0f + (clamped * 37.0f);
        enabled_ = (clamped > 0.01f);
    }

    void processBlock(float* buffer, size_t frameCount, size_t channels) {
        if (!enabled_) return;

        for (size_t f = 0; f < frameCount; ++f) {
            float maxAbs = 0.0f;
            for (size_t ch = 0; ch < channels; ++ch) {
                maxAbs = std::max(maxAbs, std::abs(buffer[f * channels + ch]));
            }

            // Envelope detection
            if (maxAbs > envelope_) {
                envelope_ = attackCoeff_ * (envelope_ - maxAbs) + maxAbs;
            } else {
                envelope_ = releaseCoeff_ * (envelope_ - maxAbs) + maxAbs;
            }

            float envDb = (envelope_ > 0.00001f) ? (20.0f * std::log10(envelope_)) : -100.0f;
            
            // Downward expansion curve
            float targetGain = 1.0f;
            if (envDb < thresholdDb_) {
                float diffDb = thresholdDb_ - envDb;
                float attenuationDb = diffDb * 1.5f; // expansion ratio
                targetGain = std::pow(10.0f, -attenuationDb / 20.0f);
            }

            // Smooth gain changes to avoid audio clicks
            currentGain_ += (targetGain - currentGain_) * 0.05f;

            for (size_t ch = 0; ch < channels; ++ch) {
                buffer[f * channels + ch] *= currentGain_;
            }
        }
    }

private:
    void updateTimeConstants() {
        attackCoeff_ = std::exp(-1.0f / (sampleRate_ * 0.002f));  // fast attack (2ms)
        releaseCoeff_ = std::exp(-1.0f / (sampleRate_ * 0.080f)); // release (80ms)
    }

    float sampleRate_ = 44100.0f;
    float thresholdDb_ = -50.0f;
    bool enabled_ = true;

    float envelope_ = 0.0f;
    float currentGain_ = 1.0f;
    float attackCoeff_ = 0.0f;
    float releaseCoeff_ = 0.0f;
};

} // namespace DSP
} // namespace Audiee
