# react-native-audiee 🎙️⚡

> **High-Performance C++ Audio Engine with Multi-Track Timeline & Real-Time DSP for React Native.**

[![License: MIT](https://img.shields.io/badge/License-MIT-orange.svg)](https://opensource.org/licenses/MIT)
[![Platform - iOS & Android](https://img.shields.io/badge/Platform-iOS%20%7C%20Android-blue.svg)](https://reactnative.dev)
[![Engine - miniaudio](https://img.shields.io/badge/Audio%20Core-miniaudio%20C%2B%2B17-green.svg)](https://miniaud.in)
[![Package Manager - SPM & CocoaPods](https://img.shields.io/badge/Package%20Manager-SPM%20%2B%20CocoaPods-purple.svg)](https://swift.org/package-manager/)

`react-native-audiee` brings DAW-level audio processing to React Native. Built on a low-latency C++17 core (`miniaudio`), it provides sample-accurate multi-track mixing, zero-latency DSP sliders (EQ, Freeverb, Compressor, Noise Gate), high-speed waveform peak extraction, and Dolby On-inspired high-fidelity recording with live metering.

---

## ✨ Features

- **⚡ C++ Low-Latency Audio Core**: Powered by `miniaudio.h` directly accessing AAudio/OpenSL ES on Android and CoreAudio on iOS.
- **🎛️ Real-Time DSP Chain**:
  - **3-Band Biquad Equalizer** (Low warmth shelf, Presence peak, Air brilliance shelf)
  - **Freeverb Stereo Reverb** (8 comb filters + 4 allpass filters)
  - **Dynamic Range Compressor & Master Peak Limiter** (Prevents clipping over 0 dBFS)
  - **Noise Gate / Room Purifier** (Downward expander to remove AC hum and background hiss)
- **🎙️ Dolby On-Style Recorder**:
  - High-fidelity mono/stereo uncompressed PCM capture with live RMS and peak dB VU metering callback.
- **📊 60 FPS Waveform Peak Extraction**:
  - Decodes audio files in C++ and generates downsampled min/max peak buffers optimized for `@shopify/react-native-skia` or custom Canvas renderers.
- **🎚️ Multi-Track Timeline**:
  - Sample-accurate playback, independent track faders, panning, mute/solo, sub-millisecond timeline time offsets, and offline mix export to WAV / MP3 with LUFS loudness normalization.
- **🍏 Swift Package Manager & CocoaPods**:
  - Future-proof dual support: Works seamlessly via `Package.swift` in modern Xcode / React Native 0.74+ or via CocoaPods.
- **🧩 Extensible Architecture**:
  - Clean C++ and TypeScript hooks for proprietary add-on modules (e.g. `react-native-audiee-pro` for sub-sample cross-correlation alignment and AI STFT stem separation).

---

## 📦 Installation

```bash
npm install react-native-audiee
# or
yarn add react-native-audiee
```

### Expo Setup (Managed & Bare Workflow)

`react-native-audiee` includes a built-in **Expo Config Plugin** that automatically configures microphone permissions and background audio capabilities during `npx expo prebuild`.

In your `app.json`:
```json
{
  "expo": {
    "plugins": [
      [
        "react-native-audiee",
        {
          "microphonePermission": "Allow Audiee to record studio-quality audio."
        }
      ]
    ]
  }
}
```

Then build your custom dev client:
```bash
npx expo run:ios
# or
npx expo run:android
```

### iOS Setup (Choose CocoaPods or Swift Package Manager)

#### Option A: CocoaPods
```bash
cd ios && pod install
```

#### Option B: Swift Package Manager (SPM)
In Xcode:
1. Go to **File → Add Package Dependencies...**
2. Enter the repository URL: `https://github.com/iphonic/react-native-audiee.git`
3. Select version rule **Up to Next Major Version**.

---

## 🚀 Quick Start

### 1. Initialize the Audio Engine
```typescript
import { AudieeEngine, BuiltinPresets } from 'react-native-audiee';

const engine = new AudieeEngine({ sampleRate: 44100 });
await engine.initialize();
```

### 2. Multi-Track Timeline & Real-Time DSP
```typescript
// Add audio tracks to the timeline
const vocal = await engine.addTrack('track_vocal', 'file:///path/to/vocal.wav');
const beat = await engine.addTrack('track_beat', 'file:///path/to/beat.mp3');

// Apply built-in creator presets
vocal.applyPreset(BuiltinPresets.PodcastCrisp);

// Or tweak granular DSP sliders in real-time with zero latency
vocal.setDSP({
  denoise: 0.70,        // 70% background room noise reduction
  warmthDb: 3.5,        // +3.5 dB low-end warmth
  airDb: 4.0,           // +4.0 dB high-frequency presence
  reverbMix: 0.15,      // 15% studio room space
  compRatio: 4.0,       // 4:1 vocal compression
  compThresholdDb: -18,
});

// Adjust track placement, volume and pan
vocal.setVolume(1.0);
beat.setVolume(0.75);
beat.setPan(-0.2); // Pan slightly left

// Start synchronized multi-track playback
await engine.play();
```

### 3. High-Speed Waveform Peak Extraction
Extract peaks in milliseconds for silky smooth 60 FPS waveform rendering:
```typescript
const waveform = await AudieeEngine.extractWaveform('file:///path/to/vocal.wav', 50);
console.log('Total peaks extracted:', waveform.minPeaks.length);
console.log('Audio duration (s):', waveform.durationSeconds);
```

### 4. Dolby On-Style Recording with Live Metering
```typescript
await engine.startRecording('file:///path/to/my_recording.wav');

// Stop and retrieve file
await engine.stopRecording();
```

### 5. Export Mastered Mix
```typescript
await engine.exportMix({
  outputPath: 'file:///path/to/mastered_mix.wav',
  normalizeLufs: -14.0, // Spotify & social media loudness target
});
```

---

## 💎 Pro Addon (`react-native-audiee-pro`)

Proprietary advanced features are modularized in the companion `react-native-audiee-pro` package:
- **1-Touch Smart Alignment**: Sub-sample phase locking via fast Cross-Correlation.
- **AI Stem Separator**: STFT / iSTFT Overlap-Add spectral masking for isolating vocals from backing tracks.

To integrate, install `@audiee/pro` and pass your `AudieeEngine` instance:
```typescript
import { autoAlignTracks, separateStems } from 'react-native-audiee-pro';

// Synchronize tracks with sub-sample cross-correlation
const { offsetMs, confidence } = await autoAlignTracks(engine, 'track_vocal', 'track_beat');

// AI Vocal / Instrumental separation
const { vocalPath, instrumentalPath } = await separateStems('song.mp3');
```

---

## 🧪 Testing

Run the native C++ unit tests locally:
```bash
npm run test:cpp
```

Typecheck the TypeScript codebase:
```bash
npm run typecheck
```

---

## 📱 Running the Example App

The `example/` app demonstrates the full 5-screen Audiee production workflow: Dolby On-style recording with live VU metering, creator presets, granular DSP sliders, and multi-track studio timeline.

### Option A: Run with Expo (Recommended)
```bash
cd example
yarn install

# Run on iOS Simulator (auto-generates native project & compiles C++ engine)
npx expo run:ios

# Or run on Android Emulator / Device
npx expo run:android

# Start Metro dev server
npx expo start --dev-client
```

### Option B: Run with Bare React Native CLI
```bash
cd example
yarn install

# iOS:
cd ios && pod install && cd ..
yarn ios:bare

# Android:
yarn android:bare
```

---

## 📄 License

MIT © [Audiee Team](https://github.com/iphonic/react-native-audiee)
