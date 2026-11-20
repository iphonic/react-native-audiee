export interface WaveformData {
  minPeaks: number[];
  maxPeaks: number[];
  durationSeconds: number;
  sampleRate: number;
  channels: number;
}

export interface AlignmentResult {
  offsetMs: number;
  confidence: number; // 0.0 to 1.0
}

export interface DSPSettings {
  denoise?: number; // 0.0 to 1.0
  warmthDb?: number; // -12.0 to +12.0 dB
  airDb?: number; // -12.0 to +12.0 dB
  reverbMix?: number; // 0.0 to 1.0
  reverbRoomSize?: number; // 0.0 to 1.0
  compRatio?: number; // 1.0 to 20.0
  compThresholdDb?: number; // -60.0 to 0.0 dB
}

export interface Preset {
  id: string;
  name: string;
  icon: string;
  description: string;
  isCustom?: boolean;
  dsp: DSPSettings;
}

export interface ExportOptions {
  outputPath: string;
  format?: 'wav' | 'mp3' | 'm4a' | 'flac';
  sampleRate?: number;
  bitrateKbps?: number;
  normalizeLufs?: number; // e.g. -14.0 for Spotify, -16.0 for Podcasts
}

export interface SeparationResult {
  success: boolean;
  vocalPath: string;
  instrumentalPath: string;
  durationSeconds: number;
  errorMessage?: string;
}

export type SeparationProgressCallback = (progress: number) => void;

export type MeteringCallback = (rmsDb: number, peakDb: number) => void;
