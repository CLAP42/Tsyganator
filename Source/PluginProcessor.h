#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "DSP/JunoVoice.h"
#include "DSP/JunoChorus.h"
#include "DSP/JunoLFO.h"
#include "DSP/JunoPresets.h"
#include "DSP/StepSequencer.h"
#include "DSP/Arpeggiator.h"
#include "DSP/SamplePlayer.h"
#include "DSP/VintageProcessor.h"
#include "PresetManager.h"
#include <random>
#include <atomic>
#include <cstdint>

/**
 * TsyganatorProcessor — Main audio processor
 * 6-voice Juno synth + LFO + 16-step sequencer + preset management
 * Two personality modes: HARD BELGIAN MODE / SAD ITALIAN MODE
 * Four play modes: Off / ARP / SEQ SYNTH / SEQ SAMPLE
 */
class TsyganatorProcessor : public juce::AudioProcessor
{
public:
    enum SynthMode { BelgianMode = 0, ItalianMode = 1 };
    enum PlayMode  { ModeOff = 0, ModeArp = 1, ModeSeqSynth = 2, ModeSeqSample = 3 };

    TsyganatorProcessor();
    ~TsyganatorProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Tsyganator"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    double getTailLengthSeconds() const override { return 2.0; }

    int getNumPrograms() override { return (int)currentModePresets.size(); }
    int getCurrentProgram() override { return currentPreset; }
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    // Parameter tree
    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // Cached parameter pointers (P2-1/P2-2): avoids per-block string hash lookup
    // in processBlock and updateVoiceParams. Initialized in the constructor.
    struct CachedParams {
        std::atomic<float>* sawLevel        = nullptr;
        std::atomic<float>* pulseLevel      = nullptr;
        std::atomic<float>* triangleLevel   = nullptr;
        std::atomic<float>* subLevel        = nullptr;
        std::atomic<float>* noiseLevel      = nullptr;
        std::atomic<float>* pulseWidth      = nullptr;
        std::atomic<float>* osc1Volume      = nullptr;
        std::atomic<float>* osc2Saw         = nullptr;
        std::atomic<float>* osc2Pulse       = nullptr;
        std::atomic<float>* osc2Triangle    = nullptr;
        std::atomic<float>* osc2PW          = nullptr;
        std::atomic<float>* osc2Octave      = nullptr;
        std::atomic<float>* osc2Fine        = nullptr;
        std::atomic<float>* osc2Volume      = nullptr;
        std::atomic<float>* cutoff          = nullptr;
        std::atomic<float>* resonance       = nullptr;
        std::atomic<float>* filterEnvAmount = nullptr;
        std::atomic<float>* filterAttack    = nullptr;
        std::atomic<float>* filterDecay     = nullptr;
        std::atomic<float>* filterSustain   = nullptr;
        std::atomic<float>* filterRelease   = nullptr;
        std::atomic<float>* ampAttack       = nullptr;
        std::atomic<float>* ampDecay        = nullptr;
        std::atomic<float>* ampSustain      = nullptr;
        std::atomic<float>* ampRelease      = nullptr;
        std::atomic<float>* chorusMode      = nullptr;
        std::atomic<float>* masterGain      = nullptr;
        std::atomic<float>* lfoRate         = nullptr;
        std::atomic<float>* lfoDepth        = nullptr;
        std::atomic<float>* lfoWaveform     = nullptr;
        std::atomic<float>* lfoDestination  = nullptr;
        std::atomic<float>* lfoSync         = nullptr;
        std::atomic<float>* lfoSyncRate     = nullptr;
        std::atomic<float>* unisonMode      = nullptr;
        std::atomic<float>* unisonDetune    = nullptr;
        std::atomic<float>* keyTracking     = nullptr;
        std::atomic<float>* portamento      = nullptr;
        std::atomic<float>* seqSwing        = nullptr;
        std::atomic<float>* seqGateLength   = nullptr;
        std::atomic<float>* seqNumSteps     = nullptr;
        std::atomic<float>* seqRate         = nullptr;
        std::atomic<float>* arpRate         = nullptr;
        std::atomic<float>* arpMode         = nullptr;
        std::atomic<float>* arpGateLength   = nullptr;
        std::atomic<float>* arpOctaveRange  = nullptr;
        std::atomic<float>* globalFineTune  = nullptr;
        std::atomic<float>* vintageMode     = nullptr;
        std::atomic<float>* vintageAmount   = nullptr;
        std::atomic<float>* stereoSpread    = nullptr;
        std::atomic<float>* velocityCurve   = nullptr;
    };
    CachedParams params;

    // Personality mode switching (Belgian / Italian)
    void setMode(SynthMode newMode);
    SynthMode getMode() const { return currentMode; }
    const std::vector<TsyganatorPreset>& getCurrentPresets() const { return currentModePresets; }
    void loadPreset(int index);

    // Play mode (Off / ARP / SEQ SYNTH / SEQ SAMPLE)
    void setPlayMode(PlayMode newMode);
    PlayMode getPlayMode() const { return playMode; }

    // Convenience queries for editor
    bool isSequencerActive() const { return playMode == ModeSeqSynth || playMode == ModeSeqSample; }
    bool isArpActive() const { return playMode == ModeArp; }

    // Sequencer access
    StepSequencer& getSequencer() { return sequencer; }

    // Arpeggiator access
    Arpeggiator& getArpeggiator() { return arpeggiator; }

    // Sample player access
    SamplePlayer& getSamplePlayer() { return samplePlayer; }
    bool loadSampleFile(const juce::File& file) { return samplePlayer.loadFile(file); }
    void clearSample() { samplePlayer.clearSample(); }

    // Tsyganize — randomize modulation parameters for unique sound
    void tsyganize();

    // UI feedback — audio-thread-safe reads
    float getPeakLevel() const { return peakLevel.load(std::memory_order_relaxed); }
    double getLastNoteOnTime() const { return lastNoteOnTime.load(std::memory_order_relaxed); }

    // Preset manager access
    PresetManager& getPresetManager() { return *presetManager; }

    // Mode listener
    struct ModeListener { virtual ~ModeListener() = default; virtual void modeChanged(SynthMode) = 0; };
    void addModeListener(ModeListener* l) { modeListeners.push_back(l); }
    void removeModeListener(ModeListener* l) {
        modeListeners.erase(std::remove(modeListeners.begin(), modeListeners.end(), l), modeListeners.end());
    }

private:
    static constexpr int NUM_VOICES = 6;
    JunoVoice voices[NUM_VOICES];
    JunoChorus chorus;
    JunoLFO lfo;
    StepSequencer sequencer;
    Arpeggiator arpeggiator;
    SamplePlayer samplePlayer;
    VintageProcessor vintage;
    PlayMode playMode = ModeOff;

    std::unique_ptr<PresetManager> presetManager;

    SynthMode currentMode = ItalianMode;
    std::vector<TsyganatorPreset> currentModePresets;
    int currentPreset = 0;
    std::vector<ModeListener*> modeListeners;

    // Voice management
    int voiceStealIndex = 0;     // legacy fallback when no candidate found
    uint64_t voiceAgeCounter = 0; // monotonically increasing, stamped at each noteOn
    int lastPlayedNote = -1;

    // RNG for Tsyganize
    std::mt19937 rng{ std::random_device{}() };

    // Audio-thread-safe values for UI feedback (features 3 & 8)
    std::atomic<float> peakLevel{0.0f};       // current output peak for mascot reactivity
    std::atomic<double> lastNoteOnTime{0.0};   // timestamp of last MIDI note-on for LED blink

    // Host transport tracking (for one-shot killAllNotes)
    bool wasHostPlaying = false;

    // 303-style keyboard transpose for sequencer
    std::atomic<int> seqTransposeOffset{0};    // semitones offset from keyboard input
    int seqBaseNote = 60;                       // C4 = reference root for transpose calculation
    int seqTransposeHeldNote = -1;              // currently held note for transpose (track releases)

    // Pitch bend and mod wheel state
    float pitchBendSemitones = 0.0f;           // current pitch bend in semitones (-2..+2)
    float modWheelValue = 0.0f;                // mod wheel CC1 (0..1) → cutoff modulation

    // MIDI output for sequencer/arp → DAW recording
    juce::MidiBuffer pendingMidiOut;
    int currentRenderSample = 0;

    // Unison note tracking (for correct release)
    int lastUnisonNote = -1;

    void handleMidiMessage(const juce::MidiMessage& msg);
    void handleSequencerNoteOn(int note, float velocity);
    void handleSequencerNoteOff(int note);
    void updateVoiceParams();
    void killAllNotes();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TsyganatorProcessor)
};
