#pragma once
#define _USE_MATH_DEFINES
#include <cmath>

namespace Audiee {
namespace DSP {

enum class FilterType {
    LowPass,
    HighPass,
    BandPass,
    Notch,
    PeakingEQ,
    LowShelf,
    HighShelf
};

/**
 * Standard Biquad Filter based on Robert Bristow-Johnson's Audio EQ Cookbook.
 * Direct Form II Transposed for numerical stability and zero audio clicks.
 */
class BiquadFilter {
public:
    BiquadFilter() {
        reset();
    }

    void reset() {
        z1 = 0.0f;
        z2 = 0.0f;
        b0 = 1.0f;
        b1 = 0.0f;
        b2 = 0.0f;
        a1 = 0.0f;
        a2 = 0.0f;
    }

    void configure(FilterType type, float frequency, float sampleRate, float Q, float gainDb = 0.0f) {
        if (sampleRate <= 0.0f) sampleRate = 44100.0f;
        if (frequency <= 0.0f) frequency = 20.0f;
        if (frequency >= sampleRate * 0.49f) frequency = sampleRate * 0.49f;
        if (Q <= 0.0f) Q = 0.7071f;

        const float omega = 2.0f * (float)M_PI * frequency / sampleRate;
        const float sinOmega = std::sin(omega);
        const float cosOmega = std::cos(omega);
        const float alpha = sinOmega / (2.0f * Q);
        const float A = std::pow(10.0f, gainDb / 40.0f);

        float a0 = 1.0f;

        switch (type) {
            case FilterType::PeakingEQ: {
                b0 = 1.0f + alpha * A;
                b1 = -2.0f * cosOmega;
                b2 = 1.0f - alpha * A;
                a0 = 1.0f + alpha / A;
                a1 = -2.0f * cosOmega;
                a2 = 1.0f - alpha / A;
                break;
            }
            case FilterType::LowShelf: {
                const float twoSqrtAAlpha = 2.0f * std::sqrt(A) * alpha;
                b0 = A * ((A + 1.0f) - (A - 1.0f) * cosOmega + twoSqrtAAlpha);
                b1 = 2.0f * A * ((A - 1.0f) - (A + 1.0f) * cosOmega);
                b2 = A * ((A + 1.0f) - (A - 1.0f) * cosOmega - twoSqrtAAlpha);
                a0 = (A + 1.0f) + (A - 1.0f) * cosOmega + twoSqrtAAlpha;
                a1 = -2.0f * ((A - 1.0f) + (A + 1.0f) * cosOmega);
                a2 = (A + 1.0f) + (A - 1.0f) * cosOmega - twoSqrtAAlpha;
                break;
            }
            case FilterType::HighShelf: {
                const float twoSqrtAAlpha = 2.0f * std::sqrt(A) * alpha;
                b0 = A * ((A + 1.0f) + (A - 1.0f) * cosOmega + twoSqrtAAlpha);
                b1 = -2.0f * A * ((A - 1.0f) + (A + 1.0f) * cosOmega);
                b2 = A * ((A + 1.0f) + (A - 1.0f) * cosOmega - twoSqrtAAlpha);
                a0 = (A + 1.0f) - (A - 1.0f) * cosOmega + twoSqrtAAlpha;
                a1 = 2.0f * ((A - 1.0f) - (A + 1.0f) * cosOmega);
                a2 = (A + 1.0f) - (A - 1.0f) * cosOmega - twoSqrtAAlpha;
                break;
            }
            case FilterType::LowPass: {
                b0 = (1.0f - cosOmega) / 2.0f;
                b1 = 1.0f - cosOmega;
                b2 = (1.0f - cosOmega) / 2.0f;
                a0 = 1.0f + alpha;
                a1 = -2.0f * cosOmega;
                a2 = 1.0f - alpha;
                break;
            }
            case FilterType::HighPass: {
                b0 = (1.0f + cosOmega) / 2.0f;
                b1 = -(1.0f + cosOmega);
                b2 = (1.0f + cosOmega) / 2.0f;
                a0 = 1.0f + alpha;
                a1 = -2.0f * cosOmega;
                a2 = 1.0f - alpha;
                break;
            }
            default:
                break;
        }

        // Normalize coefficients
        const float invA0 = 1.0f / a0;
        b0 *= invA0;
        b1 *= invA0;
        b2 *= invA0;
        a1 *= invA0;
        a2 *= invA0;
    }

    inline float process(float in) {
        float out = in * b0 + z1;
        z1 = in * b1 - a1 * out + z2;
        z2 = in * b2 - a2 * out;
        return out;
    }

private:
    float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f;
    float a1 = 0.0f, a2 = 0.0f;
    float z1 = 0.0f, z2 = 0.0f;
};

/**
 * 3-Band Equalizer tailored for Vocal & Music enhancement
 * - Low Shelf (Warmth / Body at ~200Hz)
 * - Peaking Mid (Presence / Clarity at ~2.5kHz)
 * - High Shelf (Air / Brilliance at ~10kHz)
 */
class Equalizer3Band {
public:
    void setup(float sampleRate) {
        sampleRate_ = sampleRate;
        updateFilters();
    }

    void setWarmth(float gainDb) {
        warmthDb_ = gainDb;
        lowShelf_.configure(FilterType::LowShelf, 220.0f, sampleRate_, 0.707f, warmthDb_);
    }

    void setPresence(float gainDb) {
        presenceDb_ = gainDb;
        midPeak_.configure(FilterType::PeakingEQ, 2500.0f, sampleRate_, 1.0f, presenceDb_);
    }

    void setAir(float gainDb) {
        airDb_ = gainDb;
        highShelf_.configure(FilterType::HighShelf, 9000.0f, sampleRate_, 0.707f, airDb_);
    }

    void processBlock(float* samples, size_t frameCount, size_t channels) {
        for (size_t i = 0; i < frameCount * channels; ++i) {
            float s = samples[i];
            s = lowShelf_.process(s);
            s = midPeak_.process(s);
            s = highShelf_.process(s);
            samples[i] = s;
        }
    }

private:
    void updateFilters() {
        setWarmth(warmthDb_);
        setPresence(presenceDb_);
        setAir(airDb_);
    }

    float sampleRate_ = 44100.0f;
    float warmthDb_ = 0.0f;
    float presenceDb_ = 0.0f;
    float airDb_ = 0.0f;

    BiquadFilter lowShelf_;
    BiquadFilter midPeak_;
    BiquadFilter highShelf_;
};

} // namespace DSP
} // namespace Audiee
