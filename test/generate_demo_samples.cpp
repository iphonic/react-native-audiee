#define MINIAUDIO_IMPLEMENTATION
#include "../cpp/miniaudio.h"
#include <cmath>
#include <vector>
#include <iostream>

int main() {
    const uint32_t sampleRate = 44100;
    const double duration = 4.0; // 4 seconds
    const size_t totalFrames = static_cast<size_t>(sampleRate * duration);

    // 1. Generate Demo Vocal (Melodic harmonic tones)
    {
        std::vector<float> vocal(totalFrames * 2, 0.0f); // Stereo
        for (size_t i = 0; i < totalFrames; ++i) {
            double t = (double)i / sampleRate;
            // Vocal melody sequence: 440Hz (A4) -> 523Hz (C5) -> 587Hz (D5) -> 659Hz (E5)
            double freq = 440.0;
            if (t > 1.0 && t <= 2.0) freq = 523.25;
            else if (t > 2.0 && t <= 3.0) freq = 587.33;
            else if (t > 3.0) freq = 659.25;

            // Voice harmonics
            float s = 0.5f * std::sin(2.0 * M_PI * freq * t)
                    + 0.25f * std::sin(2.0 * M_PI * freq * 2.0 * t)
                    + 0.12f * std::sin(2.0 * M_PI * freq * 3.0 * t);

            // Envelope (fade in and out)
            float env = std::sin(M_PI * (t / duration));
            s *= env;

            vocal[i * 2] = s;
            vocal[i * 2 + 1] = s;
        }

        ma_encoder_config config = ma_encoder_config_init(ma_encoding_format_wav, ma_format_f32, 2, sampleRate);
        ma_encoder encoder;
        ma_encoder_init_file("example/assets/demo_vocal.wav", &config, &encoder);
        ma_encoder_write_pcm_frames(&encoder, vocal.data(), totalFrames, nullptr);
        ma_encoder_uninit(&encoder);
        std::cout << "Generated example/assets/demo_vocal.wav" << std::endl;
    }

    // 2. Generate Demo Beat (Kick and snare beats intentionally delayed by 150ms for Auto-Align testing)
    {
        std::vector<float> beat(totalFrames * 2, 0.0f);
        const double offsetSec = 0.150; // 150ms offset

        for (size_t i = 0; i < totalFrames; ++i) {
            double t = (double)i / sampleRate;
            if (t < offsetSec) continue;

            double localT = t - offsetSec;
            double beatFrac = std::fmod(localT, 0.5); // beat every 0.5s (120 BPM)

            float s = 0.0f;
            if (beatFrac < 0.1) {
                // Kick drum transient
                double freq = 120.0 * std::exp(-beatFrac * 30.0);
                s = 0.8f * std::sin(2.0 * M_PI * freq * beatFrac);
            }

            beat[i * 2] = s;
            beat[i * 2 + 1] = s;
        }

        ma_encoder_config config = ma_encoder_config_init(ma_encoding_format_wav, ma_format_f32, 2, sampleRate);
        ma_encoder encoder;
        ma_encoder_init_file("example/assets/demo_beat.wav", &config, &encoder);
        ma_encoder_write_pcm_frames(&encoder, beat.data(), totalFrames, nullptr);
        ma_encoder_uninit(&encoder);
        std::cout << "Generated example/assets/demo_beat.wav" << std::endl;
    }

    return 0;
}
