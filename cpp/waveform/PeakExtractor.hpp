#pragma once
#include <string>
#include <vector>
#include <algorithm>
#include <cmath>
#include "../miniaudio.h"

namespace Audiee {
namespace Waveform {

struct WaveformData {
    std::vector<float> minPeaks;
    std::vector<float> maxPeaks;
    double durationSeconds = 0.0;
    uint32_t sampleRate = 0;
    uint32_t channels = 0;
};

class PeakExtractor {
public:
    /**
     * Decodes an audio file (WAV, MP3, FLAC) and extracts amplitude peaks
     * at a specified resolution (points per second).
     */
    static WaveformData extractPeaks(const std::string& filePath, uint32_t pointsPerSecond = 50) {
        WaveformData result;

        ma_decoder_config config = ma_decoder_config_init(ma_format_f32, 1, 0); // decode as mono float32
        ma_decoder decoder;

        ma_result res = ma_decoder_init_file(filePath.c_str(), &config, &decoder);
        if (res != MA_SUCCESS) {
            return result;
        }

        result.sampleRate = decoder.outputSampleRate;
        result.channels = decoder.outputChannels;

        ma_uint64 totalFrames = 0;
        ma_decoder_get_length_in_pcm_frames(&decoder, &totalFrames);
        result.durationSeconds = static_cast<double>(totalFrames) / result.sampleRate;

        if (pointsPerSecond == 0) pointsPerSecond = 50;
        const size_t framesPerPoint = std::max<size_t>(1, result.sampleRate / pointsPerSecond);

        std::vector<float> readBuffer(4096);
        float currentMin = 0.0f;
        float currentMax = 0.0f;
        size_t frameAcc = 0;

        while (true) {
            ma_uint64 framesRead = 0;
            res = ma_decoder_read_pcm_frames(&decoder, readBuffer.data(), readBuffer.size(), &framesRead);
            if (res != MA_SUCCESS || framesRead == 0) {
                break;
            }

            for (size_t i = 0; i < framesRead; ++i) {
                float sample = readBuffer[i];
                if (sample < currentMin) currentMin = sample;
                if (sample > currentMax) currentMax = sample;

                if (++frameAcc >= framesPerPoint) {
                    result.minPeaks.push_back(currentMin);
                    result.maxPeaks.push_back(currentMax);
                    currentMin = 0.0f;
                    currentMax = 0.0f;
                    frameAcc = 0;
                }
            }
        }

        // Flush remaining samples if any
        if (frameAcc > 0) {
            result.minPeaks.push_back(currentMin);
            result.maxPeaks.push_back(currentMax);
        }

        ma_decoder_uninit(&decoder);
        return result;
    }
};

} // namespace Waveform
} // namespace Audiee
