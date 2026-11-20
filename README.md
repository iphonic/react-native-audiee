# react-native-audiee 🎙️⚡

> **High-Performance C++ Audio Engine with Multi-Track Timeline, Real-Time DSP, AI Stem Separation & Smart Auto-Alignment for React Native.**

[![License: MIT](https://img.shields.io/badge/License-MIT-orange.svg)](https://opensource.org/licenses/MIT)
[![Platform - iOS & Android](https://img.shields.io/badge/Platform-iOS%20%7C%20Android-blue.svg)](https://reactnative.dev)
[![Engine - miniaudio](https://img.shields.io/badge/Audio%20Core-miniaudio%20C%2B%2B17-green.svg)](https://miniaud.in)
[![Package Manager - SPM & CocoaPods](https://img.shields.io/badge/Package%20Manager-SPM%20%2B%20CocoaPods-purple.svg)](https://swift.org/package-manager/)

`react-native-audiee` brings DAW-level audio processing to React Native. Built on a low-latency C++17 core (`miniaudio`), it provides sample-accurate multi-track mixing, zero-latency DSP sliders (EQ, Freeverb, Compressor, Noise Gate), high-speed waveform peak extraction, **AI Stem Separation** (vocals vs. instrumental), and 1-touch **Smart Auto-Alignment** via cross-correlation.

---

## ✨ Features

- **⚡ C++ Low-Latency Audio Core**: Powered by `miniaudio.h` directly accessing AAudio/OpenSL ES on Android and CoreAudio on iOS.
- **🤖 AI Stem Separation**:
  - Built-in Short-Time Fourier Transform (**STFT**) and Inverse STFT (**iSTFT**) with Overlap-Add (OLA) energy normalization.
  - Decomposes mixed audio into **Isolated Vocals** and **Clean Instrumentals** (karaoke / backing track) with verified reconstruction fidelity ($< 0.005$ error).
- **🎛️ Real-Time DSP Chain**:
  - **3-Band Biquad Equalizer** (Low warmth shelf, Presence peak, Air brilliance shelf)
  - **Freeverb Stereo Reverb** (8 comb filters + 4 allpass filters)
  - **Dynamic Range Compressor & Peak Limiter** (Prevents clipping over 0 dBFS)
  - **Noise Gate / Room Purifier** (Downward expander to remove AC hum and background hiss)
- **🎯 1-Touch Smart Auto-Alignment (Pro Feature)**:
  - Uses normalized **Fast Cross-Correlation (FFT)** to automatically synchronize misaligned audio tracks (e.g. backing track to vocal, or dual mics) down to the millisecond.
- **📊 60 FPS Waveform Peak Extraction**:
  - Decodes audio files in C++ and generates downsampled min/max peak buffers optimized for `@shopify/react-native-skia`.
- **🎚️ Multi-Track Timeline**:
  - Sample-accurate playback, independent track faders, panning, mute/solo, and offline mix export to WAV / MP3 with LUFS loudness normalization.
- **🍏 Swift Package Manager & CocoaPods**:
  - Future-proof dual support: Works seamlessly via `Package.swift` in modern Xcode / React Native 0.74+ or via `react-native-audiee.podspec`.

---

## 📦 Installation

```bash
npm install react-native-audiee
# or
yarn add react-native-audiee
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

### 2. AI Stem Separation (Vocals vs Instrumental Isolation)
Isolate vocals and instrumental backing tracks with a single call:
```typescript
const { success, vocalPath, instrumentalPath } = await AudieeEngine.separateStems(
  'file:///path/to/song.mp3',
  {
    outputVocalPath: 'file:///path/to/song_vocals.wav',
    outputInstPath: 'file:///path/to/song_instrumental.wav',
  }
);

if (success) {
  // Load both stems directly onto your multi-track timeline!
  const vocalTrack = await engine.addTrack('vocals', vocalPath);
  const instTrack = await engine.addTrack('instrumental', instrumentalPath);

  // Apply vocal preset & adjust levels
  vocalTrack.applyPreset(BuiltinPresets.StudioVocal);
  instTrack.setVolume(0.85);
}
```

### 3. Multi-Track Timeline & Real-Time DSP
```typescript
// Add tracks to timeline
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

// Start multi-track playback
await engine.play();
```

### 4. 1-Touch Smart Auto-Alignment
Automatically align an out-of-sync backing beat or second mic to the lead vocal:
```typescript
const { offsetMs, confidence } = await engine.autoAlignTracks('track_vocal', 'track_beat');

console.log(`Aligned! Shifted beat by ${offsetMs}ms with ${(confidence * 100).toFixed(0)}% confidence`);

// Snap the track on the timeline
beat.setTimeOffset(offsetMs);
```

### 5. High-Speed Waveform Peak Extraction
Extract peaks in milliseconds for silky smooth 60 FPS waveform rendering:
```typescript
const waveform = await AudieeEngine.extractWaveform('file:///path/to/vocal.wav', 50);
console.log('Total peaks extracted:', waveform.minPeaks.length);
```

### 6. Export Mastered Mix
```typescript
await engine.exportMix({
  outputPath: 'file:///path/to/mastered_mix.wav',
  normalizeLufs: -14.0, // Spotify & social media loudness target
});
```

---

## 🏛️ System Architecture

```
┌────────────────────────────────────────────────────────┐
│             React Native Application (TypeScript)      │
│      Waveform Canvas (Skia) • Sliders • Transport      │
└──────────────────────────┬─────────────────────────────┘
                           │ JSI TurboModule Calls
┌──────────────────────────▼─────────────────────────────┐
│                 Audiee Core C++ Engine                 │
│  ┌───────────────────────┐  ┌───────────────────────┐  │
│  │   MultiTrackMixer     │  │   Real-Time DSP Chain │  │
│  │ • Sample-accurate sync│  │ • 3-Band Biquad EQ    │  │
│  │ • Volume & Panning    │  │ • Freeverb Reverb     │  │
│  │ • Offline file export │  │ • Compressor & Limiter│  │
│  └───────────────────────┘  │ • Room Noise Gate     │  │
│  ┌───────────────────────┐  └───────────────────────┘  │
│  │ Cross-Correlation FFT │  ┌───────────────────────┐  │
│  │ • Auto-Align Algorithm│  │  AI Stem Separator    │  │
│  └───────────────────────┘  │ • STFT / iSTFT Engine │  │
│  ┌───────────────────────┐  │ • Vocal Soft-Masking  │  │
│  │ Waveform Peak Extract │  └───────────────────────┘  │
│  └───────────────────────┘                             │
└──────────────────────────┬─────────────────────────────┘
                           │ miniaudio Driver (Direct Hardware I/O)
            [iOS: CoreAudio]   [Android: AAudio / Oboe]
```

---

## 🏷️ Versioning & Releases

This project follows [Semantic Versioning (SemVer)](https://semver.org/). To release a new version with automated podspec synchronization:

```bash
npm run version:patch   # 1.0.0 -> 1.0.1 (Bug fixes)
npm run version:minor   # 1.0.0 -> 1.1.0 (New features)
npm run version:major   # 1.0.0 -> 2.0.0 (Breaking changes)
```
The `postversion` hook automatically synchronizes the new version into `ios/react-native-audiee.podspec`, commits the changes, creates the git tag, and pushes to GitHub.

---

## 🧪 Testing the C++ Core Locally

You can compile and run the engine test suite directly on macOS or Linux:

```bash
npm run test:cpp
```

---

## 📄 License

MIT License © [Audiee](https://github.com/iphonic/react-native-audiee)
