import type { TurboModule } from 'react-native';
import { TurboModuleRegistry } from 'react-native';

export interface Spec extends TurboModule {
  // Engine Lifecycle
  initialize(sampleRate: number): Promise<boolean>;
  shutdown(): void;

  // Playback & Recording
  play(): Promise<boolean>;
  pause(): Promise<boolean>;
  stop(): void;
  seekTo(seconds: number): void;
  getCurrentTime(): Promise<number>;
  startRecording(destinationPath: string): Promise<boolean>;
  stopRecording(): Promise<boolean>;

  // Multi-Track Timeline
  addTrack(trackId: string, filePath: string): Promise<boolean>;
  removeTrack(trackId: string): Promise<boolean>;
  setTrackTimeOffset(trackId: string, offsetMs: number): void;
  setTrackVolume(trackId: string, volume: number): void;
  setTrackPan(trackId: string, pan: number): void;
  setTrackMute(trackId: string, mute: boolean): void;
  setTrackSolo(trackId: string, solo: boolean): void;

  // Real-Time DSP Sliders
  setTrackDSP(
    trackId: string,
    denoise: number,
    warmthDb: number,
    airDb: number,
    reverbMix: number,
    reverbRoomSize: number,
    compRatio: number,
    compThresholdDb: number
  ): void;

  // Smart Auto-Alignment (Pro Feature)
  autoAlignTracks(
    refTrackId: string,
    targetTrackId: string
  ): Promise<{ offsetMs: number; confidence: number }>;

  // Waveform Peak Extraction
  extractWaveform(
    filePath: string,
    pointsPerSecond: number
  ): Promise<{
    minPeaks: number[];
    maxPeaks: number[];
    durationSeconds: number;
    sampleRate: number;
  }>;

  // Mixdown & Export
  exportMix(
    outputPath: string,
    normalizeLufs: number
  ): Promise<boolean>;

  // AI Stem Separator (Vocals vs Instrumental Isolation)
  separateStems(
    inputFilePath: string,
    outputVocalPath: string,
    outputInstPath: string
  ): Promise<{
    success: boolean;
    vocalPath: string;
    instrumentalPath: string;
    durationSeconds: number;
    errorMessage?: string;
  }>;
}

export default TurboModuleRegistry.get<Spec>('AudieeEngine');
