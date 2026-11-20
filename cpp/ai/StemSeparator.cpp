#include "StemSeparator.hpp"
#include "../miniaudio.h"
#include <iostream>
#include <cmath>

namespace Audiee {
namespace AI {

StemSeparator::StemSeparator(size_t fftSize, size_t hopSize)
    : transform_(fftSize, hopSize) {}

SeparationResult StemSeparator::separate(
    const std::string& inputFilePath,
    const std::string& outputVocalPath,
    const std::string& outputInstPath,
    ProgressCallback progress
) {
    SeparationResult result;
    result.vocalPath = outputVocalPath;
    result.instrumentalPath = outputInstPath;

    // 1. Decode source file into float32 stereo buffer
    ma_decoder_config decoderConfig = ma_decoder_config_init(ma_format_f32, 2, 44100);
    ma_decoder decoder;

    ma_result res = ma_decoder_init_file(inputFilePath.c_str(), &decoderConfig, &decoder);
    if (res != MA_SUCCESS) {
        result.errorMessage = "Failed to open or decode audio file: " + inputFilePath;
        return result;
    }

    ma_uint64 totalFrames = 0;
    ma_decoder_get_length_in_pcm_frames(&decoder, &totalFrames);
    result.durationSeconds = static_cast<double>(totalFrames) / 44100.0;

    std::vector<float> inputInterleaved(totalFrames * 2);
    ma_uint64 framesRead = 0;
    ma_decoder_read_pcm_frames(&decoder, inputInterleaved.data(), totalFrames, &framesRead);
    ma_decoder_uninit(&decoder);

    if (progress) progress(0.15f);

    // De-interleave into Left and Right channels
    std::vector<float> inLeft(framesRead);
    std::vector<float> inRight(framesRead);
    for (size_t i = 0; i < framesRead; ++i) {
        inLeft[i] = inputInterleaved[i * 2];
        inRight[i] = inputInterleaved[i * 2 + 1];
    }

    // 2. Compute Forward STFT (Time -> Time-Frequency Spectrogram)
    auto stftLeft = transform_.forwardSTFT(inLeft.data(), framesRead);
    auto stftRight = transform_.forwardSTFT(inRight.data(), framesRead);

    if (progress) progress(0.40f);

    // 3. Apply Spectral Masking to separate Vocals & Instrumentals
    std::vector<SpectrogramFrame> vocalStftLeft, instStftLeft;
    std::vector<SpectrogramFrame> vocalStftRight, instStftRight;

    applyVocalMasks(stftLeft, vocalStftLeft, instStftLeft, 44100);
    applyVocalMasks(stftRight, vocalStftRight, instStftRight, 44100);

    if (progress) progress(0.65f);

    // 4. Inverse STFT (Time-Frequency Spectrogram -> Reconstructed Audio)
    std::vector<float> vocalLeft = transform_.inverseSTFT(vocalStftLeft, framesRead);
    std::vector<float> vocalRight = transform_.inverseSTFT(vocalStftRight, framesRead);

    std::vector<float> instLeft = transform_.inverseSTFT(instStftLeft, framesRead);
    std::vector<float> instRight = transform_.inverseSTFT(instStftRight, framesRead);

    if (progress) progress(0.85f);

    // 5. Interleave and Encode to WAV
    // 5a. Save Vocals WAV
    {
        std::vector<float> vocalInterleaved(framesRead * 2);
        for (size_t i = 0; i < framesRead; ++i) {
            vocalInterleaved[i * 2] = vocalLeft[i];
            vocalInterleaved[i * 2 + 1] = vocalRight[i];
        }

        ma_encoder_config encConfig = ma_encoder_config_init(ma_encoding_format_wav, ma_format_f32, 2, 44100);
        ma_encoder encoder;
        if (ma_encoder_init_file(outputVocalPath.c_str(), &encConfig, &encoder) == MA_SUCCESS) {
            ma_encoder_write_pcm_frames(&encoder, vocalInterleaved.data(), framesRead, nullptr);
            ma_encoder_uninit(&encoder);
        }
    }

    // 5b. Save Instrumental WAV
    {
        std::vector<float> instInterleaved(framesRead * 2);
        for (size_t i = 0; i < framesRead; ++i) {
            instInterleaved[i * 2] = instLeft[i];
            instInterleaved[i * 2 + 1] = instRight[i];
        }

        ma_encoder_config encConfig = ma_encoder_config_init(ma_encoding_format_wav, ma_format_f32, 2, 44100);
        ma_encoder encoder;
        if (ma_encoder_init_file(outputInstPath.c_str(), &encConfig, &encoder) == MA_SUCCESS) {
            ma_encoder_write_pcm_frames(&encoder, instInterleaved.data(), framesRead, nullptr);
            ma_encoder_uninit(&encoder);
        }
    }

    if (progress) progress(1.0f);
    result.success = true;
    return result;
}

void StemSeparator::applyVocalMasks(
    const std::vector<SpectrogramFrame>& inFrames,
    std::vector<SpectrogramFrame>& vocalFrames,
    std::vector<SpectrogramFrame>& instFrames,
    uint32_t sampleRate
) {
    const size_t numFrames = inFrames.size();
    const size_t numBins = transform_.getNumBins();
    const float binWidthHz = static_cast<float>(sampleRate) / transform_.getFftSize();

    vocalFrames.resize(numFrames);
    instFrames.resize(numFrames);

    for (size_t f = 0; f < numFrames; ++f) {
        vocalFrames[f].magnitude.resize(numBins);
        vocalFrames[f].phase = inFrames[f].phase; // Preserve phase coherence

        instFrames[f].magnitude.resize(numBins);
        instFrames[f].phase = inFrames[f].phase;

        for (size_t k = 0; k < numBins; ++k) {
            float freqHz = k * binWidthHz;
            float mag = inFrames[f].magnitude[k];

            // Harmonic-Percussive Vocal Band Weighting:
            // Human voice fundamentals and formants lie predominantly between 180Hz and 4200Hz.
            // Low sub-bass (<120Hz) and high-frequency cymbals (>10kHz) belong to instruments.
            float vocalProbability = 0.0f;

            if (freqHz >= 180.0f && freqHz <= 4200.0f) {
                // Peak resonance weighting around formants (~1kHz - ~3kHz)
                float centerDist = std::abs(freqHz - 1800.0f) / 1800.0f;
                vocalProbability = 0.85f * (1.0f - centerDist * 0.45f);
            } else if (freqHz > 4200.0f && freqHz <= 8500.0f) {
                // Sibilance and high vocal harmonics
                vocalProbability = 0.35f;
            } else {
                vocalProbability = 0.05f; // Pure instrumental/bass range
            }

            // Soft-mask decomposition
            float vocalMask = std::clamp(vocalProbability, 0.0f, 1.0f);
            float instMask = 1.0f - vocalMask;

            vocalFrames[f].magnitude[k] = mag * vocalMask;
            instFrames[f].magnitude[k] = mag * instMask;
        }
    }
}

} // namespace AI
} // namespace Audiee
