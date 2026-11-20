import NativeAudiee, { type Spec } from './NativeAudieeSpec';
import type { DSPSettings, Preset, AlignmentResult, WaveformData, ExportOptions } from './types';

export class AudioTrackHandle {
  constructor(private engine: AudieeEngine, public readonly id: string) {}

  public async setTimeOffset(offsetMs: number): Promise<void> {
    NativeAudiee?.setTrackTimeOffset(this.id, offsetMs);
  }

  public setVolume(volume: number): void {
    NativeAudiee?.setTrackVolume(this.id, Math.max(0, Math.min(2.0, volume)));
  }

  public setPan(pan: number): void {
    NativeAudiee?.setTrackPan(this.id, Math.max(-1.0, Math.min(1.0, pan)));
  }

  public setMute(mute: boolean): void {
    NativeAudiee?.setTrackMute(this.id, mute);
  }

  public setSolo(solo: boolean): void {
    NativeAudiee?.setTrackSolo(this.id, solo);
  }

  public setDSP(dsp: DSPSettings): void {
    NativeAudiee?.setTrackDSP(
      this.id,
      dsp.denoise ?? 0.0,
      dsp.warmthDb ?? 0.0,
      dsp.airDb ?? 0.0,
      dsp.reverbMix ?? 0.0,
      dsp.reverbRoomSize ?? 0.5,
      dsp.compRatio ?? 1.0,
      dsp.compThresholdDb ?? -16.0
    );
  }

  public applyPreset(preset: Preset): void {
    this.setDSP(preset.dsp);
  }

  public async remove(): Promise<boolean> {
    return (await NativeAudiee?.removeTrack(this.id)) ?? false;
  }
}

export class AudieeEngine {
  private sampleRate: number;
  private tracks: Map<string, AudioTrackHandle> = new Map();

  constructor(options?: { sampleRate?: number }) {
    this.sampleRate = options?.sampleRate ?? 44100;
  }

  public async initialize(): Promise<boolean> {
    return (await NativeAudiee?.initialize(this.sampleRate)) ?? false;
  }

  public shutdown(): void {
    NativeAudiee?.shutdown();
    this.tracks.clear();
  }

  public async play(): Promise<boolean> {
    return (await NativeAudiee?.play()) ?? false;
  }

  public async pause(): Promise<boolean> {
    return (await NativeAudiee?.pause()) ?? false;
  }

  public stop(): void {
    NativeAudiee?.stop();
  }

  public seekTo(seconds: number): void {
    NativeAudiee?.seekTo(seconds);
  }

  public async getCurrentTime(): Promise<number> {
    return (await NativeAudiee?.getCurrentTime()) ?? 0.0;
  }

  public async startRecording(destinationPath: string): Promise<boolean> {
    return (await NativeAudiee?.startRecording(destinationPath)) ?? false;
  }

  public async stopRecording(): Promise<boolean> {
    return (await NativeAudiee?.stopRecording()) ?? false;
  }

  public async addTrack(trackId: string, filePath: string): Promise<AudioTrackHandle | null> {
    const success = await NativeAudiee?.addTrack(trackId, filePath);
    if (success) {
      const handle = new AudioTrackHandle(this, trackId);
      this.tracks.set(trackId, handle);
      return handle;
    }
    return null;
  }

  public getTrack(trackId: string): AudioTrackHandle | undefined {
    return this.tracks.get(trackId);
  }

  /**
   * Pro Feature: 1-Touch Smart Alignment
   * Automatically calculates the millisecond time offset needed to lock targetTrack to refTrack.
   */
  public async autoAlignTracks(
    refTrackId: string,
    targetTrackId: string
  ): Promise<AlignmentResult> {
    const result = await NativeAudiee?.autoAlignTracks(refTrackId, targetTrackId);
    return result ?? { offsetMs: 0, confidence: 0 };
  }

  /**
   * Fast Waveform Extraction for 60fps Skia Rendering
   */
  public static async extractWaveform(
    filePath: string,
    pointsPerSecond: number = 50
  ): Promise<WaveformData> {
    const data = await NativeAudiee?.extractWaveform(filePath, pointsPerSecond);
    return {
      minPeaks: data?.minPeaks ?? [],
      maxPeaks: data?.maxPeaks ?? [],
      durationSeconds: data?.durationSeconds ?? 0,
      sampleRate: data?.sampleRate ?? 44100,
      channels: 1,
    };
  }

  /**
   * AI Stem Separator: Isolates mixed audio into Vocals and Instrumental stems
   */
  public static async separateStems(
    inputFilePath: string,
    options?: {
      outputVocalPath?: string;
      outputInstPath?: string;
    }
  ): Promise<{
    success: boolean;
    vocalPath: string;
    instrumentalPath: string;
    durationSeconds: number;
    errorMessage?: string;
  }> {
    const vocalPath = options?.outputVocalPath ?? inputFilePath.replace(/\.[^/.]+$/, '_vocals.wav');
    const instPath = options?.outputInstPath ?? inputFilePath.replace(/\.[^/.]+$/, '_instrumental.wav');

    const result = await NativeAudiee?.separateStems(inputFilePath, vocalPath, instPath);
    return (
      result ?? {
        success: false,
        vocalPath,
        instrumentalPath: instPath,
        durationSeconds: 0,
        errorMessage: 'Native Audiee engine not initialized',
      }
    );
  }

  /**
   * Offline Mixdown and Export to file (WAV / MP3)
   */
  public async exportMix(options: ExportOptions): Promise<boolean> {
    return (
      (await NativeAudiee?.exportMix(
        options.outputPath,
        options.normalizeLufs ?? -14.0
      )) ?? false
    );
  }
}
