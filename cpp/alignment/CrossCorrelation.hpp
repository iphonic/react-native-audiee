#pragma once
#include <vector>
#include <cmath>
#include <algorithm>
#include <cstdint>

namespace Audiee {
namespace Alignment {

struct AlignmentResult {
    int64_t offsetMs = 0;
    float confidence = 0.0f; // 0.0 to 1.0
};

/**
 * Smart Audio Alignment Engine
 * Uses Cross-Correlation & Transient Peak Correlation to synchronize
 * out-of-phase or misaligned audio tracks automatically with millisecond accuracy.
 */
class CrossCorrelationEngine {
public:
    /**
     * Calculates the time offset (in milliseconds) needed to shift 'target' so it locks with 'reference'.
     * A positive offsetMs means target should be moved forward (delayed).
     * A negative offsetMs means target should be moved backward.
     */
    static AlignmentResult calculateOffset(
        const float* refBuffer, size_t refFrames,
        const float* targetBuffer, size_t targetFrames,
        uint32_t sampleRate,
        int64_t maxSearchWindowMs = 2500
    ) {
        if (!refBuffer || !targetBuffer || refFrames == 0 || targetFrames == 0 || sampleRate == 0) {
            return { 0, 0.0f };
        }

        // 1. Downsample both signals to ~8000Hz for high-speed mobile computation
        const uint32_t targetDownsampleRate = 8000;
        const size_t downsampleFactor = std::max<size_t>(1, sampleRate / targetDownsampleRate);
        const float effectiveSampleRate = static_cast<float>(sampleRate) / downsampleFactor;

        std::vector<float> refDs = downsampleAndEnvelope(refBuffer, refFrames, downsampleFactor);
        std::vector<float> targetDs = downsampleAndEnvelope(targetBuffer, targetFrames, downsampleFactor);

        if (refDs.empty() || targetDs.empty()) {
            return { 0, 0.0f };
        }

        // 2. Limit correlation length to the first ~10 seconds of both signals
        const size_t maxAnalysisSamples = static_cast<size_t>(10.0f * effectiveSampleRate);
        if (refDs.size() > maxAnalysisSamples) refDs.resize(maxAnalysisSamples);
        if (targetDs.size() > maxAnalysisSamples) targetDs.resize(maxAnalysisSamples);

        const int maxLagSamples = static_cast<int>((maxSearchWindowMs * 0.001f) * effectiveSampleRate);

        // 3. Compute Normalized Cross-Correlation over the search window
        float maxCorrelation = -1.0f;
        int bestLag = 0;
        float totalEnergy = 0.0f;

        for (int lag = -maxLagSamples; lag <= maxLagSamples; ++lag) {
            float sum = 0.0f;
            float refEnergy = 0.0f;
            float targetEnergy = 0.0f;

            size_t startRef = (lag >= 0) ? lag : 0;
            size_t startTarget = (lag >= 0) ? 0 : -lag;
            size_t overlapLen = std::min(refDs.size() - startRef, targetDs.size() - startTarget);

            if (overlapLen < static_cast<size_t>(effectiveSampleRate * 0.5f)) {
                continue; // Need at least 0.5 seconds of overlap
            }

            for (size_t i = 0; i < overlapLen; ++i) {
                float r = refDs[startRef + i];
                float t = targetDs[startTarget + i];
                sum += r * t;
                refEnergy += r * r;
                targetEnergy += t * t;
            }

            float denom = std::sqrt(refEnergy * targetEnergy);
            if (denom > 0.00001f) {
                float normCorr = sum / denom;
                if (normCorr > maxCorrelation) {
                    maxCorrelation = normCorr;
                    bestLag = lag;
                }
            }
        }

        // 4. Convert lag sample count to milliseconds
        int64_t offsetMs = static_cast<int64_t>((static_cast<float>(bestLag) / effectiveSampleRate) * 1000.0f);
        float confidence = std::clamp(maxCorrelation, 0.0f, 1.0f);

        return { offsetMs, confidence };
    }

private:
    static std::vector<float> downsampleAndEnvelope(const float* buffer, size_t frames, size_t factor) {
        std::vector<float> out;
        out.reserve(frames / factor);

        for (size_t i = 0; i < frames; i += factor) {
            float maxVal = 0.0f;
            size_t end = std::min(frames, i + factor);
            for (size_t j = i; j < end; ++j) {
                maxVal = std::max(maxVal, std::abs(buffer[j]));
            }
            out.push_back(maxVal);
        }

        // Apply a gentle moving average smoothing filter to isolate transients
        if (out.size() > 4) {
            for (size_t i = 2; i < out.size() - 2; ++i) {
                out[i] = (out[i - 2] + out[i - 1] * 2.0f + out[i] * 3.0f + out[i + 1] * 2.0f + out[i + 2]) / 9.0f;
            }
        }

        return out;
    }
};

} // namespace Alignment
} // namespace Audiee
