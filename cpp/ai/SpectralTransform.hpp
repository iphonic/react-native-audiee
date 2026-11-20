#pragma once
#include <vector>
#include <complex>
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace Audiee {
namespace AI {

using Complex = std::complex<float>;

struct SpectrogramFrame {
    std::vector<float> magnitude;
    std::vector<float> phase;
};

/**
 * High-Performance C++ STFT & iSTFT Spectral Transformation Engine
 * Performs windowed Fast Fourier Transform & Overlap-Add (OLA) reconstruction
 * for audio source separation (Vocals vs Instrumental).
 */
class SpectralTransform {
public:
    SpectralTransform(size_t fftSize = 2048, size_t hopSize = 512)
        : fftSize_(fftSize), hopSize_(hopSize) {
        initWindow();
    }

    size_t getFftSize() const { return fftSize_; }
    size_t getHopSize() const { return hopSize_; }
    size_t getNumBins() const { return (fftSize_ / 2) + 1; }

    /**
     * Forward STFT: Time-Domain PCM -> Time-Frequency Spectrogram
     */
    std::vector<SpectrogramFrame> forwardSTFT(const float* signal, size_t numSamples) {
        std::vector<SpectrogramFrame> frames;
        if (!signal || numSamples < fftSize_) return frames;

        const size_t numFrames = (numSamples - fftSize_) / hopSize_ + 1;
        frames.reserve(numFrames);

        std::vector<Complex> buffer(fftSize_);

        for (size_t f = 0; f < numFrames; ++f) {
            size_t sampleOffset = f * hopSize_;

            // Apply Hann Window
            for (size_t i = 0; i < fftSize_; ++i) {
                buffer[i] = Complex(signal[sampleOffset + i] * window_[i], 0.0f);
            }

            // In-place Radix-2 FFT
            fft(buffer.data(), fftSize_);

            // Extract Magnitude and Phase for bins [0, fftSize/2]
            SpectrogramFrame frame;
            const size_t numBins = getNumBins();
            frame.magnitude.resize(numBins);
            frame.phase.resize(numBins);

            for (size_t k = 0; k < numBins; ++k) {
                frame.magnitude[k] = std::abs(buffer[k]);
                frame.phase[k] = std::arg(buffer[k]);
            }

            frames.push_back(std::move(frame));
        }

        return frames;
    }

    /**
     * Inverse STFT (iSTFT): Magnitude & Phase Spectrogram -> Reconstructed Time-Domain PCM
     */
    std::vector<float> inverseSTFT(const std::vector<SpectrogramFrame>& frames, size_t originalLength) {
        std::vector<float> output(originalLength, 0.0f);
        std::vector<float> windowSum(originalLength, 0.0f);

        if (frames.empty()) return output;

        std::vector<Complex> buffer(fftSize_);
        const size_t numBins = getNumBins();

        for (size_t f = 0; f < frames.size(); ++f) {
            const auto& frame = frames[f];
            size_t sampleOffset = f * hopSize_;
            if (sampleOffset + fftSize_ > originalLength) break;

            // Reconstruct Hermitian symmetric complex spectrum
            for (size_t k = 0; k < numBins; ++k) {
                float mag = frame.magnitude[k];
                float phase = frame.phase[k];
                buffer[k] = std::polar(mag, phase);
            }
            // Conjugate symmetry for negative frequencies
            for (size_t k = numBins; k < fftSize_; ++k) {
                buffer[k] = std::conj(buffer[fftSize_ - k]);
            }

            // In-place Inverse FFT
            ifft(buffer.data(), fftSize_);

            // Overlap-Add (OLA) with synthesis window
            for (size_t i = 0; i < fftSize_; ++i) {
                float win = window_[i];
                output[sampleOffset + i] += buffer[i].real() * win;
                windowSum[sampleOffset + i] += win * win;
            }
        }

        // Normalize by accumulated window energy to prevent amplitude distortion
        for (size_t i = 0; i < originalLength; ++i) {
            if (windowSum[i] > 1e-4f) {
                output[i] /= windowSum[i];
            }
        }

        return output;
    }

private:
    void initWindow() {
        window_.resize(fftSize_);
        for (size_t i = 0; i < fftSize_; ++i) {
            // Periodic Hann Window
            window_[i] = 0.5f * (1.0f - std::cos(2.0f * (float)M_PI * (float)i / (float)fftSize_));
        }
    }

    // Radix-2 Cooley-Tukey Decimation-in-Time FFT
    static void fft(Complex* data, size_t n) {
        if (n <= 1) return;

        // Bit-reversal permutation
        size_t j = 0;
        for (size_t i = 0; i < n - 1; ++i) {
            if (i < j) std::swap(data[i], data[j]);
            size_t k = n / 2;
            while (k <= j) {
                j -= k;
                k /= 2;
            }
            j += k;
        }

        // Butterfly computations
        for (size_t len = 2; len <= n; len <<= 1) {
            float angle = -2.0f * (float)M_PI / (float)len;
            Complex wlen(std::cos(angle), std::sin(angle));
            for (size_t i = 0; i < n; i += len) {
                Complex w(1.0f, 0.0f);
                for (size_t j_idx = 0; j_idx < len / 2; ++j_idx) {
                    Complex u = data[i + j_idx];
                    Complex v = data[i + j_idx + len / 2] * w;
                    data[i + j_idx] = u + v;
                    data[i + j_idx + len / 2] = u - v;
                    w *= wlen;
                }
            }
        }
    }

    static void ifft(Complex* data, size_t n) {
        // Conjugate input
        for (size_t i = 0; i < n; ++i) {
            data[i] = std::conj(data[i]);
        }
        // Forward FFT on conjugated data
        fft(data, n);
        // Conjugate output and scale by 1/N
        const float scale = 1.0f / (float)n;
        for (size_t i = 0; i < n; ++i) {
            data[i] = std::conj(data[i]) * scale;
        }
    }

    size_t fftSize_ = 2048;
    size_t hopSize_ = 512;
    std::vector<float> window_;
};

} // namespace AI
} // namespace Audiee
