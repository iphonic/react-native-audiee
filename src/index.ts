export * from './types';
export * from './AudieeEngine';

import type { Preset } from './types';

export const BuiltinPresets: Record<string, Preset> = {
  PodcastCrisp: {
    id: 'builtin-podcast-crisp',
    name: 'Podcast Crisp',
    icon: '🎙️',
    description: 'Crystal clear voice with plosive damping & room de-reverb',
    dsp: {
      denoise: 0.70,
      warmthDb: 3.0,
      airDb: 4.0,
      reverbMix: 0.12,
      reverbRoomSize: 0.20,
      compRatio: 4.0,
      compThresholdDb: -18.0,
    },
  },
  WarmVoiceover: {
    id: 'builtin-warm-voiceover',
    name: 'Warm Voiceover',
    icon: '🎤',
    description: 'Deep proximity boost for intimate radio host presence',
    dsp: {
      denoise: 0.60,
      warmthDb: 6.0,
      airDb: 2.0,
      reverbMix: 0.10,
      reverbRoomSize: 0.15,
      compRatio: 3.5,
      compThresholdDb: -20.0,
    },
  },
  StudioVocal: {
    id: 'builtin-studio-vocal',
    name: 'Studio Vocal',
    icon: '✨',
    description: 'Silky high-end air with rich stereo space and leveling',
    dsp: {
      denoise: 0.40,
      warmthDb: 2.0,
      airDb: 5.5,
      reverbMix: 0.32,
      reverbRoomSize: 0.65,
      compRatio: 4.5,
      compThresholdDb: -16.0,
    },
  },
  NoisePurifier: {
    id: 'builtin-noise-purifier',
    name: 'Noise Purifier',
    icon: '🧹',
    description: 'Aggressive AC hum, fan and background room noise suppression',
    dsp: {
      denoise: 0.95,
      warmthDb: 0.0,
      airDb: 2.0,
      reverbMix: 0.0,
      reverbRoomSize: 0.0,
      compRatio: 2.5,
      compThresholdDb: -22.0,
    },
  },
  AcousticBeat: {
    id: 'builtin-acoustic-beat',
    name: 'Acoustic / Beat',
    icon: '🎸',
    description: 'Punchy transient dynamics with subtle harmonic low-end warmth',
    dsp: {
      denoise: 0.25,
      warmthDb: 4.0,
      airDb: 4.0,
      reverbMix: 0.20,
      reverbRoomSize: 0.40,
      compRatio: 3.0,
      compThresholdDb: -14.0,
    },
  },
};
