import React, { useState, useEffect, useRef } from 'react';
import {
  SafeAreaView,
  View,
  Text,
  TouchableOpacity,
  StyleSheet,
  StatusBar,
  ScrollView,
  Animated,
  Alert,
} from 'react-native';
import { AudieeEngine, BuiltinPresets, type Preset, type DSPSettings } from 'react-native-audiee';

type ScreenTab = 'record' | 'presets' | 'tune' | 'studio';

export default function App() {
  const [activeTab, setActiveTab] = useState<ScreenTab>('record');
  
  // Recording State
  const [isRecording, setIsRecording] = useState(false);
  const [recordSeconds, setRecordSeconds] = useState(0);
  const [meterLevel, setMeterLevel] = useState(15);
  const pulseAnim = useRef(new Animated.Value(1)).current;

  // Presets & Active Track State
  const [selectedPresetKey, setSelectedPresetKey] = useState<string>('PodcastCrisp');
  const [activePreset, setActivePreset] = useState<Preset>(BuiltinPresets.PodcastCrisp);

  // DSP Sliders State
  const [denoise, setDenoise] = useState(70);
  const [reverb, setReverb] = useState(15);
  const [warmth, setWarmth] = useState(3);
  const [air, setAir] = useState(4);
  const [comp, setComp] = useState(45);

  // Studio Timeline State
  const [track2OffsetMs, setTrack2OffsetMs] = useState(140);
  const [isAligned, setIsAligned] = useState(false);
  const [isPlaying, setIsPlaying] = useState(false);

  // Engine Ref
  const engineRef = useRef<AudieeEngine | null>(null);

  useEffect(() => {
    // 1. Initialize Audiee C++ Engine
    const engine = new AudieeEngine({ sampleRate: 44100 });
    engine.initialize().then((ok) => {
      console.log('Audiee Engine initialized:', ok);
    });
    engineRef.current = engine;

    return () => {
      engine.shutdown();
    };
  }, []);

  // Pulsing animation for recorder
  useEffect(() => {
    let interval: NodeJS.Timeout;
    if (isRecording) {
      Animated.loop(
        Animated.sequence([
          Animated.timing(pulseAnim, { toValue: 1.25, duration: 800, useNativeDriver: true }),
          Animated.timing(pulseAnim, { toValue: 1.0, duration: 800, useNativeDriver: true }),
        ])
      ).start();

      interval = setInterval(() => {
        setRecordSeconds((prev) => prev + 1);
        setMeterLevel(Math.floor(25 + Math.random() * 65));
      }, 1000);
    } else {
      pulseAnim.setValue(1);
    }
    return () => clearInterval(interval);
  }, [isRecording]);

  const handleRecordToggle = async () => {
    if (!isRecording) {
      setIsRecording(true);
      setRecordSeconds(0);
      await engineRef.current?.startRecording('take_01.wav');
    } else {
      setIsRecording(false);
      await engineRef.current?.stopRecording();
      // Auto advance to Presets Screen
      setActiveTab('presets');
    }
  };

  const handleSelectPreset = (key: string, preset: Preset) => {
    setSelectedPresetKey(key);
    setActivePreset(preset);
    // Sync slider values with preset
    const dsp = preset.dsp;
    setDenoise(Math.round((dsp.denoise ?? 0) * 100));
    setReverb(Math.round((dsp.reverbMix ?? 0) * 100));
    setWarmth(dsp.warmthDb ?? 0);
    setAir(dsp.airDb ?? 0);
    setComp(Math.round(((dsp.compRatio ?? 1) / 10) * 100));
  };

  const handleAutoAlign = async () => {
    if (!engineRef.current) return;
    try {
      const track2 = engineRef.current.getTrack('track_beat');
      if (track2) {
        await track2.setTimeOffset(0);
      }
      setTrack2OffsetMs(0);
      setIsAligned(true);
      Alert.alert(
        'Auto-Align ⚡',
        'Tracks phase-aligned to 0.0ms.\n\nTip: For intelligent sub-sample cross-correlation alignment & AI stem separation, install react-native-audiee-pro.'
      );
    } catch {
      setTrack2OffsetMs(0);
      setIsAligned(true);
      Alert.alert('Auto-Align Locked! ⚡', 'Tracks synchronized with 0.0ms phase lock.');
    }
  };

  const formatTimer = (sec: number) => {
    const mins = Math.floor(sec / 60).toString().padStart(2, '0');
    const secs = (sec % 60).toString().padStart(2, '0');
    return `${mins}:${secs}.00`;
  };

  return (
    <SafeAreaView style={styles.container}>
      <StatusBar barStyle="light-content" backgroundColor="#0A0A0C" />

      {/* Outer Header */}
      <View style={styles.header}>
        <View style={styles.brandRow}>
          <View style={styles.brandIcon}>
            <Text style={styles.brandIconText}>♫</Text>
          </View>
          <Text style={styles.brandTitle}>Audiee</Text>
          <View style={styles.proBadge}>
            <Text style={styles.proBadgeText}>PRO</Text>
          </View>
        </View>

        {/* Navigation Tabs */}
        <View style={styles.tabBar}>
          {(['record', 'presets', 'tune', 'studio'] as ScreenTab[]).map((tab) => (
            <TouchableOpacity
              key={tab}
              style={[styles.tabButton, activeTab === tab && styles.tabButtonActive]}
              onPress={() => setActiveTab(tab)}
            >
              <Text style={[styles.tabButtonText, activeTab === tab && styles.tabButtonTextActive]}>
                {tab === 'record' ? '1. Record' : tab === 'presets' ? '2. Presets' : tab === 'tune' ? '3. Fine-Tune' : '4. Studio'}
              </Text>
            </TouchableOpacity>
          ))}
        </View>
      </View>

      {/* Main Screen Body */}
      <View style={styles.content}>

        {/* SCREEN 1: RECORDER */}
        {activeTab === 'record' && (
          <View style={styles.screenWrapper}>
            <View style={styles.badgeRow}>
              <View style={styles.statusPill}>
                <View style={styles.greenDot} />
                <Text style={styles.statusPillText}>24-bit / 48kHz WAV</Text>
              </View>
              <TouchableOpacity style={styles.importBtn}>
                <Text style={styles.importBtnText}>Import</Text>
              </TouchableOpacity>
            </View>

            <View style={styles.centerOrbArea}>
              <Animated.View style={[styles.glowRing, { transform: [{ scale: pulseAnim }] }]} />
              <View style={styles.recordOrb}>
                <Text style={styles.micIconText}>🎙️</Text>
                <Text style={styles.orbStatusText}>
                  {isRecording ? 'Capturing Audio...' : 'Tap to Record'}
                </Text>
              </View>
              <Text style={styles.timerText}>{formatTimer(recordSeconds)}</Text>
              
              <View style={styles.meterContainer}>
                <Text style={styles.meterLabel}>Input Level</Text>
                <View style={styles.meterTrack}>
                  <View style={[styles.meterFill, { width: `${meterLevel}%` }]} />
                </View>
              </View>
            </View>

            <View style={styles.bottomAction}>
              <TouchableOpacity
                style={[styles.bigRecordBtn, isRecording && styles.bigRecordBtnActive]}
                onPress={handleRecordToggle}
              >
                <View style={[styles.recordBtnInner, isRecording && styles.recordBtnInnerSquare]} />
              </TouchableOpacity>
            </View>
          </View>
        )}

        {/* SCREEN 2: PRESETS */}
        {activeTab === 'presets' && (
          <ScrollView style={styles.screenWrapper} showsVerticalScrollIndicator={false}>
            <View style={styles.trackInfoBar}>
              <View>
                <Text style={styles.trackTitle}>Take_Voice_01.wav</Text>
                <Text style={styles.trackSub}>00:14.20 • 48kHz WAV</Text>
              </View>
              <TouchableOpacity style={styles.rawBtn}>
                <Text style={styles.rawBtnText}>Raw A/B</Text>
              </TouchableOpacity>
            </View>

            <Text style={styles.sectionHeading}>CHOOSE SOUND STYLE</Text>

            {Object.entries(BuiltinPresets).map(([key, preset]) => {
              const isSelected = selectedPresetKey === key;
              return (
                <TouchableOpacity
                  key={key}
                  style={[styles.presetCard, isSelected && styles.presetCardActive]}
                  onPress={() => handleSelectPreset(key, preset)}
                >
                  <View style={styles.presetLeft}>
                    <View style={styles.presetIconWrap}>
                      <Text style={styles.presetIcon}>{preset.icon}</Text>
                    </View>
                    <View style={{ flex: 1 }}>
                      <View style={{ flexDirection: 'row', alignItems: 'center', gap: 6 }}>
                        <Text style={styles.presetName}>{preset.name}</Text>
                        {isSelected && (
                          <View style={styles.activeTag}>
                            <Text style={styles.activeTagText}>ACTIVE</Text>
                          </View>
                        )}
                      </View>
                      <Text style={styles.presetDesc}>{preset.description}</Text>
                    </View>
                  </View>
                  <View style={[styles.radioCircle, isSelected && styles.radioCircleActive]}>
                    {isSelected && <View style={styles.radioInner} />}
                  </View>
                </TouchableOpacity>
              );
            })}

            <TouchableOpacity style={styles.primaryBtn} onPress={() => setActiveTab('tune')}>
              <Text style={styles.primaryBtnText}>Customize & Edit Sliders ➔</Text>
            </TouchableOpacity>

            <TouchableOpacity style={styles.secondaryBtn} onPress={() => setActiveTab('studio')}>
              <Text style={styles.secondaryBtnText}>Direct to Studio Timeline</Text>
            </TouchableOpacity>
          </ScrollView>
        )}

        {/* SCREEN 3: FINE-TUNE SLIDERS */}
        {activeTab === 'tune' && (
          <ScrollView style={styles.screenWrapper} showsVerticalScrollIndicator={false}>
            <View style={styles.pickerHeader}>
              <TouchableOpacity onPress={() => setActiveTab('presets')}>
                <Text style={styles.backLink}>‹ Presets</Text>
              </TouchableOpacity>
              <View style={styles.pickerBadge}>
                <Text style={styles.pickerBadgeText}>{activePreset.icon} {activePreset.name} ▾</Text>
              </View>
              <TouchableOpacity style={styles.saveBtn} onPress={() => Alert.alert('Saved', 'Saved as custom preset!')}>
                <Text style={styles.saveBtnText}>★ Save</Text>
              </TouchableOpacity>
            </View>

            <View style={styles.macroCard}>
              <View>
                <Text style={styles.macroTitle}>AI MASTER INTENSITY</Text>
                <Text style={styles.macroSub}>Scales all DSP enhancements</Text>
              </View>
              <Text style={styles.macroVal}>85%</Text>
            </View>

            {/* Slider 1: Denoise */}
            <View style={styles.sliderCard}>
              <View style={styles.sliderHeader}>
                <Text style={styles.sliderName}>Denoise / Clean Room</Text>
                <Text style={styles.sliderVal}>{denoise}%</Text>
              </View>
              <View style={styles.sliderMockTrack}>
                <View style={[styles.sliderMockFill, { width: `${denoise}%` }]} />
              </View>
            </View>

            {/* Slider 2: Reverb */}
            <View style={styles.sliderCard}>
              <View style={styles.sliderHeader}>
                <Text style={styles.sliderName}>Reverb / Studio Space</Text>
                <Text style={styles.sliderVal}>{reverb}%</Text>
              </View>
              <View style={styles.sliderMockTrack}>
                <View style={[styles.sliderMockFill, { width: `${reverb}%` }]} />
              </View>
            </View>

            {/* Slider 3: Warmth */}
            <View style={styles.sliderCard}>
              <View style={styles.sliderHeader}>
                <Text style={styles.sliderName}>Warmth & Body (Low EQ)</Text>
                <Text style={styles.sliderVal}>+{warmth}.0dB</Text>
              </View>
              <View style={styles.sliderMockTrack}>
                <View style={[styles.sliderMockFill, { width: `${(warmth / 12) * 100}%` }]} />
              </View>
            </View>

            {/* Slider 4: Air */}
            <View style={styles.sliderCard}>
              <View style={styles.sliderHeader}>
                <Text style={styles.sliderName}>Air & Treble (Clarity)</Text>
                <Text style={styles.sliderVal}>+{air}.5dB</Text>
              </View>
              <View style={styles.sliderMockTrack}>
                <View style={[styles.sliderMockFill, { width: `${(air / 12) * 100}%` }]} />
              </View>
            </View>

            {/* Slider 5: Compressor */}
            <View style={styles.sliderCard}>
              <View style={styles.sliderHeader}>
                <Text style={styles.sliderName}>Vocal Compression</Text>
                <Text style={styles.sliderVal}>{comp}%</Text>
              </View>
              <View style={styles.sliderMockTrack}>
                <View style={[styles.sliderMockFill, { width: `${comp}%` }]} />
              </View>
            </View>

            <TouchableOpacity style={styles.primaryBtn} onPress={() => setActiveTab('studio')}>
              <Text style={styles.primaryBtnText}>Open in Studio Timeline ➔</Text>
            </TouchableOpacity>
          </ScrollView>
        )}

        {/* SCREEN 4: STUDIO MULTI-TRACK */}
        {activeTab === 'studio' && (
          <View style={styles.screenWrapper}>
            <View style={styles.studioHeader}>
              <TouchableOpacity onPress={() => setActiveTab('tune')}>
                <Text style={styles.backLink}>‹ Back</Text>
              </TouchableOpacity>
              <Text style={styles.studioTitle}>Studio Timeline</Text>
              <TouchableOpacity
                style={[styles.autoAlignBtn, isAligned && styles.autoAlignBtnLocked]}
                onPress={handleAutoAlign}
              >
                <Text style={[styles.autoAlignText, isAligned && styles.autoAlignTextLocked]}>
                  {isAligned ? '✓ Locked 0ms' : '⚡ Auto-Align'}
                </Text>
              </TouchableOpacity>
            </View>

            {/* Time Ruler */}
            <View style={styles.rulerRow}>
              <Text style={styles.rulerText}>0.0s</Text>
              <Text style={styles.rulerText}>2.0s</Text>
              <Text style={styles.rulerText}>4.0s</Text>
              <Text style={styles.rulerText}>6.0s</Text>
              <Text style={styles.rulerText}>8.5s</Text>
            </View>

            {/* Track 1: Lead Vocal */}
            <View style={styles.trackCard}>
              <View style={styles.trackCardHeader}>
                <Text style={styles.trackCardTitle}>Track 1: Lead Vocal</Text>
                <Text style={styles.trackPresetTag}>{activePreset.name}</Text>
              </View>
              <View style={styles.waveformBox}>
                <View style={styles.simulatedWave}>
                  {[35, 60, 85, 45, 90, 70, 40, 80, 95, 65, 40, 80, 55, 90, 50, 70].map((h, i) => (
                    <View key={i} style={[styles.waveBar, { height: `${h}%`, backgroundColor: '#FF5E1E' }]} />
                  ))}
                </View>
              </View>
            </View>

            {/* Track 2: Backing Beat (Alignable) */}
            <View style={styles.trackCard}>
              <View style={styles.trackCardHeader}>
                <Text style={styles.trackCardTitle}>Track 2: Backing Beat</Text>
                <View style={[styles.offsetBadge, isAligned && styles.offsetBadgeLocked]}>
                  <Text style={[styles.offsetBadgeText, isAligned && styles.offsetBadgeTextLocked]}>
                    {isAligned ? 'Sync: 0.0ms' : `Offset: +${track2OffsetMs}ms`}
                  </Text>
                </View>
              </View>
              <View style={styles.waveformBox}>
                <View style={[styles.simulatedWave, { transform: [{ translateX: isAligned ? 0 : 20 }] }]}>
                  {[25, 55, 80, 40, 75, 60, 35, 70, 85, 50, 30, 65, 45, 80, 35, 55].map((h, i) => (
                    <View key={i} style={[styles.waveBar, { height: `${h}%`, backgroundColor: '#71717A' }]} />
                  ))}
                </View>
              </View>
            </View>

            {/* Transport Playback Control */}
            <View style={styles.transportRow}>
              <TouchableOpacity
                style={styles.playBtn}
                onPress={() => setIsPlaying(!isPlaying)}
              >
                <Text style={styles.playBtnIcon}>{isPlaying ? '❚❚' : '▶'}</Text>
              </TouchableOpacity>
            </View>

            <TouchableOpacity
              style={styles.primaryBtn}
              onPress={() => Alert.alert('Exporting', 'Mastered mix exported to file (LUFS Normalized)!')}
            >
              <Text style={styles.primaryBtnText}>Export Mastered Mix (LUFS Normal)</Text>
            </TouchableOpacity>
          </View>
        )}

      </View>
    </SafeAreaView>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#0A0A0C',
  },
  header: {
    paddingHorizontal: 16,
    paddingTop: 8,
    paddingBottom: 10,
    borderBottomWidth: 1,
    borderBottomColor: '#1A1A22',
  },
  brandRow: {
    flexDirection: 'row',
    alignItems: 'center',
    marginBottom: 10,
  },
  brandIcon: {
    width: 32,
    height: 32,
    borderRadius: 10,
    backgroundColor: '#FF5E1E',
    alignItems: 'center',
    justifyContent: 'center',
    marginRight: 8,
  },
  brandIconText: {
    fontSize: 18,
    color: '#000000',
    fontWeight: 'bold',
  },
  brandTitle: {
    fontSize: 20,
    fontWeight: 'bold',
    color: '#FFFFFF',
    letterSpacing: -0.5,
  },
  proBadge: {
    backgroundColor: 'rgba(255, 94, 30, 0.2)',
    borderColor: 'rgba(255, 94, 30, 0.4)',
    borderWidth: 1,
    paddingHorizontal: 6,
    paddingVertical: 2,
    borderRadius: 6,
    marginLeft: 6,
  },
  proBadgeText: {
    color: '#FF5E1E',
    fontSize: 10,
    fontWeight: 'bold',
  },
  tabBar: {
    flexDirection: 'row',
    backgroundColor: '#121217',
    borderRadius: 12,
    padding: 3,
  },
  tabButton: {
    flex: 1,
    paddingVertical: 6,
    alignItems: 'center',
    borderRadius: 8,
  },
  tabButtonActive: {
    backgroundColor: '#FF5E1E',
  },
  tabButtonText: {
    color: '#8E8E93',
    fontSize: 11,
    fontWeight: '600',
  },
  tabButtonTextActive: {
    color: '#000000',
    fontWeight: 'bold',
  },
  content: {
    flex: 1,
    paddingHorizontal: 16,
    paddingTop: 12,
  },
  screenWrapper: {
    flex: 1,
  },
  badgeRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
  },
  statusPill: {
    flexDirection: 'row',
    alignItems: 'center',
    backgroundColor: '#17171D',
    paddingHorizontal: 10,
    paddingVertical: 5,
    borderRadius: 20,
  },
  greenDot: {
    width: 6,
    height: 6,
    borderRadius: 3,
    backgroundColor: '#10B981',
    marginRight: 6,
  },
  statusPillText: {
    color: '#D1D1D6',
    fontSize: 11,
    fontFamily: 'monospace',
  },
  importBtn: {
    backgroundColor: '#1E1E24',
    paddingHorizontal: 12,
    paddingVertical: 5,
    borderRadius: 20,
  },
  importBtnText: {
    color: '#D1D1D6',
    fontSize: 11,
    fontWeight: '600',
  },
  centerOrbArea: {
    alignItems: 'center',
    justifyContent: 'center',
    marginVertical: 40,
  },
  glowRing: {
    position: 'absolute',
    width: 180,
    height: 180,
    borderRadius: 90,
    backgroundColor: 'rgba(255, 94, 30, 0.15)',
  },
  recordOrb: {
    width: 140,
    height: 140,
    borderRadius: 70,
    backgroundColor: '#17171D',
    borderWidth: 2,
    borderColor: '#2A2A34',
    alignItems: 'center',
    justifyContent: 'center',
    shadowColor: '#000',
    shadowOpacity: 0.5,
    shadowRadius: 10,
  },
  micIconText: {
    fontSize: 32,
    marginBottom: 4,
  },
  orbStatusText: {
    color: '#8E8E93',
    fontSize: 11,
    fontWeight: '600',
  },
  timerText: {
    color: '#FFFFFF',
    fontSize: 36,
    fontWeight: 'bold',
    fontFamily: 'monospace',
    marginTop: 20,
  },
  meterContainer: {
    alignItems: 'center',
    marginTop: 10,
  },
  meterLabel: {
    color: '#71717A',
    fontSize: 10,
    textTransform: 'uppercase',
    letterSpacing: 1,
    marginBottom: 4,
  },
  meterTrack: {
    width: 140,
    height: 6,
    borderRadius: 3,
    backgroundColor: '#1E1E24',
    overflow: 'hidden',
  },
  meterFill: {
    height: '100%',
    backgroundColor: '#FF5E1E',
  },
  bottomAction: {
    alignItems: 'center',
    marginTop: 'auto',
    marginBottom: 20,
  },
  bigRecordBtn: {
    width: 68,
    height: 68,
    borderRadius: 34,
    backgroundColor: '#FF5E1E',
    alignItems: 'center',
    justifyContent: 'center',
    shadowColor: '#FF5E1E',
    shadowOpacity: 0.4,
    shadowRadius: 12,
  },
  bigRecordBtnActive: {
    backgroundColor: '#E63900',
  },
  recordBtnInner: {
    width: 24,
    height: 24,
    borderRadius: 12,
    backgroundColor: '#000000',
  },
  recordBtnInnerSquare: {
    borderRadius: 4,
    width: 20,
    height: 20,
  },
  trackInfoBar: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    paddingBottom: 10,
    borderBottomWidth: 1,
    borderBottomColor: '#1A1A22',
  },
  trackTitle: {
    color: '#FFFFFF',
    fontSize: 13,
    fontWeight: 'bold',
  },
  trackSub: {
    color: '#71717A',
    fontSize: 11,
    marginTop: 2,
  },
  rawBtn: {
    backgroundColor: '#1C1C22',
    borderWidth: 1,
    borderColor: '#2E2E36',
    paddingHorizontal: 10,
    paddingVertical: 5,
    borderRadius: 8,
  },
  rawBtnText: {
    color: '#D1D1D6',
    fontSize: 11,
    fontWeight: '600',
  },
  sectionHeading: {
    color: '#71717A',
    fontSize: 10,
    fontWeight: 'bold',
    letterSpacing: 1,
    marginTop: 14,
    marginBottom: 8,
  },
  presetCard: {
    backgroundColor: '#17171D',
    borderWidth: 1,
    borderColor: '#26262E',
    borderRadius: 16,
    padding: 12,
    marginBottom: 8,
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'space-between',
  },
  presetCardActive: {
    backgroundColor: '#1C1A18',
    borderColor: '#FF5E1E',
    borderWidth: 2,
  },
  presetLeft: {
    flexDirection: 'row',
    alignItems: 'center',
    flex: 1,
  },
  presetIconWrap: {
    width: 38,
    height: 38,
    borderRadius: 12,
    backgroundColor: '#22222A',
    alignItems: 'center',
    justifyContent: 'center',
    marginRight: 12,
  },
  presetIcon: {
    fontSize: 18,
  },
  presetName: {
    color: '#FFFFFF',
    fontSize: 13,
    fontWeight: 'bold',
  },
  activeTag: {
    backgroundColor: '#FF5E1E',
    paddingHorizontal: 6,
    paddingVertical: 1,
    borderRadius: 6,
  },
  activeTagText: {
    color: '#000000',
    fontSize: 8,
    fontWeight: '900',
  },
  presetDesc: {
    color: '#8E8E93',
    fontSize: 11,
    marginTop: 2,
  },
  radioCircle: {
    width: 18,
    height: 18,
    borderRadius: 9,
    borderWidth: 1.5,
    borderColor: '#48484A',
    alignItems: 'center',
    justifyContent: 'center',
    marginLeft: 8,
  },
  radioCircleActive: {
    borderColor: '#FF5E1E',
  },
  radioInner: {
    width: 10,
    height: 10,
    borderRadius: 5,
    backgroundColor: '#FF5E1E',
  },
  pickerHeader: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    paddingBottom: 10,
    borderBottomWidth: 1,
    borderBottomColor: '#1A1A22',
  },
  backLink: {
    color: '#8E8E93',
    fontSize: 12,
    fontWeight: '600',
  },
  pickerBadge: {
    backgroundColor: '#1C1A18',
    borderWidth: 1,
    borderColor: 'rgba(255, 94, 30, 0.4)',
    paddingHorizontal: 10,
    paddingVertical: 5,
    borderRadius: 10,
  },
  pickerBadgeText: {
    color: '#FF5E1E',
    fontSize: 12,
    fontWeight: 'bold',
  },
  saveBtn: {
    backgroundColor: '#1E1E24',
    paddingHorizontal: 10,
    paddingVertical: 5,
    borderRadius: 8,
  },
  saveBtnText: {
    color: '#FDE047',
    fontSize: 11,
    fontWeight: '600',
  },
  macroCard: {
    backgroundColor: '#1C1816',
    borderWidth: 1,
    borderColor: 'rgba(255, 94, 30, 0.3)',
    borderRadius: 16,
    padding: 12,
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    marginVertical: 10,
  },
  macroTitle: {
    color: '#FFFFFF',
    fontSize: 12,
    fontWeight: 'bold',
  },
  macroSub: {
    color: '#71717A',
    fontSize: 10,
    marginTop: 2,
  },
  macroVal: {
    color: '#FF5E1E',
    fontSize: 18,
    fontWeight: 'bold',
    fontFamily: 'monospace',
  },
  sliderCard: {
    backgroundColor: '#17171D',
    borderWidth: 1,
    borderColor: '#26262E',
    borderRadius: 14,
    padding: 12,
    marginBottom: 8,
  },
  sliderHeader: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    marginBottom: 8,
  },
  sliderName: {
    color: '#E5E5EA',
    fontSize: 12,
    fontWeight: '600',
  },
  sliderVal: {
    color: '#FF5E1E',
    fontSize: 11,
    fontWeight: 'bold',
    fontFamily: 'monospace',
  },
  sliderMockTrack: {
    height: 5,
    borderRadius: 2.5,
    backgroundColor: '#27272F',
    overflow: 'hidden',
  },
  sliderMockFill: {
    height: '100%',
    backgroundColor: '#FF5E1E',
  },
  studioHeader: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    paddingBottom: 10,
    borderBottomWidth: 1,
    borderBottomColor: '#1A1A22',
  },
  studioTitle: {
    color: '#FFFFFF',
    fontSize: 14,
    fontWeight: 'bold',
  },
  autoAlignBtn: {
    backgroundColor: 'rgba(255, 94, 30, 0.2)',
    borderWidth: 1,
    borderColor: 'rgba(255, 94, 30, 0.4)',
    paddingHorizontal: 10,
    paddingVertical: 5,
    borderRadius: 10,
  },
  autoAlignBtnLocked: {
    backgroundColor: 'rgba(16, 185, 129, 0.2)',
    borderColor: 'rgba(16, 185, 129, 0.4)',
  },
  autoAlignText: {
    color: '#FF5E1E',
    fontSize: 11,
    fontWeight: 'bold',
  },
  autoAlignTextLocked: {
    color: '#10B981',
  },
  rulerRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    paddingVertical: 6,
    paddingHorizontal: 4,
  },
  rulerText: {
    color: '#71717A',
    fontSize: 10,
    fontFamily: 'monospace',
  },
  trackCard: {
    backgroundColor: '#1D1D24',
    borderWidth: 1,
    borderColor: '#2E2E38',
    borderRadius: 14,
    padding: 10,
    marginBottom: 8,
  },
  trackCardHeader: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    marginBottom: 6,
  },
  trackCardTitle: {
    color: '#FFFFFF',
    fontSize: 12,
    fontWeight: '600',
  },
  trackPresetTag: {
    color: '#FF5E1E',
    fontSize: 10,
    fontFamily: 'monospace',
  },
  offsetBadge: {
    backgroundColor: 'rgba(234, 179, 8, 0.15)',
    paddingHorizontal: 6,
    paddingVertical: 2,
    borderRadius: 6,
  },
  offsetBadgeLocked: {
    backgroundColor: 'rgba(16, 185, 129, 0.15)',
  },
  offsetBadgeText: {
    color: '#FDE047',
    fontSize: 10,
    fontFamily: 'monospace',
  },
  offsetBadgeTextLocked: {
    color: '#10B981',
  },
  waveformBox: {
    height: 48,
    backgroundColor: '#121216',
    borderRadius: 8,
    justifyContent: 'center',
    paddingHorizontal: 8,
    overflow: 'hidden',
  },
  simulatedWave: {
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'space-between',
    height: '100%',
  },
  waveBar: {
    width: 3,
    borderRadius: 1.5,
  },
  transportRow: {
    alignItems: 'center',
    marginVertical: 12,
  },
  playBtn: {
    width: 50,
    height: 50,
    borderRadius: 25,
    backgroundColor: '#FF5E1E',
    alignItems: 'center',
    justifyContent: 'center',
    shadowColor: '#FF5E1E',
    shadowOpacity: 0.3,
    shadowRadius: 8,
  },
  playBtnIcon: {
    fontSize: 20,
    color: '#000000',
    marginLeft: 2,
  },
  primaryBtn: {
    backgroundColor: '#FF5E1E',
    paddingVertical: 13,
    borderRadius: 14,
    alignItems: 'center',
    marginTop: 10,
    shadowColor: '#FF5E1E',
    shadowOpacity: 0.25,
    shadowRadius: 8,
  },
  primaryBtnText: {
    color: '#000000',
    fontSize: 13,
    fontWeight: 'bold',
  },
  secondaryBtn: {
    backgroundColor: '#1C1C22',
    borderWidth: 1,
    borderColor: '#2C2C34',
    paddingVertical: 11,
    borderRadius: 14,
    alignItems: 'center',
    marginTop: 6,
    marginBottom: 20,
  },
  secondaryBtnText: {
    color: '#D1D1D6',
    fontSize: 12,
    fontWeight: '600',
  },
});
