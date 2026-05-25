#include "PluginProcessor.h"
#include "PluginEditor.h"

TsyganatorProcessor::TsyganatorProcessor()
    : AudioProcessor(BusesProperties()
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "PARAMETERS", createParameterLayout())
{
    currentModePresets = getItalianPresets();

    // Sequencer callbacks — route to synth or sample depending on mode
    // 303-style: apply keyboard transpose offset to sequencer notes
    // Also output MIDI for DAW recording
    sequencer.onNoteOn = [this](int note, float vel) {
        if (playMode == ModeSeqSynth)
        {
            int transposed = std::clamp(note + seqTransposeOffset.load(std::memory_order_relaxed), 0, 127);
            handleSequencerNoteOn(transposed, vel);
            // Output MIDI for DAW recording
            pendingMidiOut.addEvent(juce::MidiMessage::noteOn(1, transposed, vel), currentRenderSample);
        }
        else if (playMode == ModeSeqSample)
            samplePlayer.trigger(vel);
    };
    sequencer.onNoteOff = [this](int note) {
        if (playMode == ModeSeqSynth)
        {
            int transposed = std::clamp(note + seqTransposeOffset.load(std::memory_order_relaxed), 0, 127);
            handleSequencerNoteOff(transposed);
            // Output MIDI for DAW recording
            pendingMidiOut.addEvent(juce::MidiMessage::noteOff(1, transposed, 0.0f), currentRenderSample);
        }
    };

    // Arpeggiator callbacks — plays synth + output MIDI
    arpeggiator.onNoteOn = [this](int note, float vel) {
        handleSequencerNoteOn(note, vel);
        pendingMidiOut.addEvent(juce::MidiMessage::noteOn(1, note, vel), currentRenderSample);
    };
    arpeggiator.onNoteOff = [this](int note) {
        handleSequencerNoteOff(note);
        pendingMidiOut.addEvent(juce::MidiMessage::noteOff(1, note, 0.0f), currentRenderSample);
    };

    // Preset manager
    presetManager = std::make_unique<PresetManager>(apvts, sequencer);
    // NOTE: install factory presets synchronously. An earlier optimisation
    // (P2-8) ran this on a detached background thread to speed up plugin
    // scanning, but juce_vst3_helper introspects the plugin then destroys
    // the processor immediately, leaving the background thread with a
    // dangling pointer → segfault during VST3 manifest generation.
    // The synchronous call is fast enough on first run (only ~36 small XML
    // writes), and a no-op on subsequent runs (directory already populated).
    presetManager->installFactoryPresets();

    // P2-1/P2-2: cache parameter pointers once at construction.
    // After this, processBlock and updateVoiceParams use direct atomic loads
    // instead of string-hash lookups every block / every voice.
    params.sawLevel        = apvts.getRawParameterValue("sawLevel");
    params.pulseLevel      = apvts.getRawParameterValue("pulseLevel");
    params.triangleLevel   = apvts.getRawParameterValue("triangleLevel");
    params.subLevel        = apvts.getRawParameterValue("subLevel");
    params.noiseLevel      = apvts.getRawParameterValue("noiseLevel");
    params.pulseWidth      = apvts.getRawParameterValue("pulseWidth");
    params.osc1Volume      = apvts.getRawParameterValue("osc1Volume");
    params.osc2Saw         = apvts.getRawParameterValue("osc2Saw");
    params.osc2Pulse       = apvts.getRawParameterValue("osc2Pulse");
    params.osc2Triangle    = apvts.getRawParameterValue("osc2Triangle");
    params.osc2PW          = apvts.getRawParameterValue("osc2PW");
    params.osc2Octave      = apvts.getRawParameterValue("osc2Octave");
    params.osc2Fine        = apvts.getRawParameterValue("osc2Fine");
    params.osc2Volume      = apvts.getRawParameterValue("osc2Volume");
    params.cutoff          = apvts.getRawParameterValue("cutoff");
    params.resonance       = apvts.getRawParameterValue("resonance");
    params.filterEnvAmount = apvts.getRawParameterValue("filterEnvAmount");
    params.filterAttack    = apvts.getRawParameterValue("filterAttack");
    params.filterDecay     = apvts.getRawParameterValue("filterDecay");
    params.filterSustain   = apvts.getRawParameterValue("filterSustain");
    params.filterRelease   = apvts.getRawParameterValue("filterRelease");
    params.ampAttack       = apvts.getRawParameterValue("ampAttack");
    params.ampDecay        = apvts.getRawParameterValue("ampDecay");
    params.ampSustain      = apvts.getRawParameterValue("ampSustain");
    params.ampRelease      = apvts.getRawParameterValue("ampRelease");
    params.chorusMode      = apvts.getRawParameterValue("chorusMode");
    params.masterGain      = apvts.getRawParameterValue("masterGain");
    params.lfoRate         = apvts.getRawParameterValue("lfoRate");
    params.lfoDepth        = apvts.getRawParameterValue("lfoDepth");
    params.lfoWaveform     = apvts.getRawParameterValue("lfoWaveform");
    params.lfoDestination  = apvts.getRawParameterValue("lfoDestination");
    params.lfoSync         = apvts.getRawParameterValue("lfoSync");
    params.lfoSyncRate     = apvts.getRawParameterValue("lfoSyncRate");
    params.unisonMode      = apvts.getRawParameterValue("unisonMode");
    params.unisonDetune    = apvts.getRawParameterValue("unisonDetune");
    params.keyTracking     = apvts.getRawParameterValue("keyTracking");
    params.portamento      = apvts.getRawParameterValue("portamento");
    params.seqSwing        = apvts.getRawParameterValue("seqSwing");
    params.seqGateLength   = apvts.getRawParameterValue("seqGateLength");
    params.seqNumSteps     = apvts.getRawParameterValue("seqNumSteps");
    params.seqRate         = apvts.getRawParameterValue("seqRate");
    params.arpRate         = apvts.getRawParameterValue("arpRate");
    params.arpMode         = apvts.getRawParameterValue("arpMode");
    params.arpGateLength   = apvts.getRawParameterValue("arpGateLength");
    params.arpOctaveRange  = apvts.getRawParameterValue("arpOctaveRange");
    params.globalFineTune  = apvts.getRawParameterValue("globalFineTune");
    params.vintageMode     = apvts.getRawParameterValue("vintageMode");
    params.vintageAmount   = apvts.getRawParameterValue("vintageAmount");
    params.stereoSpread    = apvts.getRawParameterValue("stereoSpread");
    params.velocityCurve   = apvts.getRawParameterValue("velocityCurve");

    // Load first preset so the synth sounds shaped on startup
    loadPreset(0);
}

juce::AudioProcessorValueTreeState::ParameterLayout TsyganatorProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Oscillator
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("sawLevel", 1), "Saw Level", 0.0f, 1.0f, 0.8f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("pulseLevel", 1), "Pulse Level", 0.0f, 1.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("subLevel", 1), "Sub Level", 0.0f, 1.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("pulseWidth", 1), "Pulse Width", 0.05f, 0.95f, 0.5f));

    // Filter
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("cutoff", 1), "Cutoff",
        juce::NormalisableRange<float>(20.0f, 20000.0f, 0.1f, 0.3f), 10000.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("resonance", 1), "Resonance", 0.0f, 1.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("filterEnvAmount", 1), "Filter Env", 0.0f, 1.0f, 0.0f));

    // Filter ADSR
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("filterAttack", 1), "Filter Attack",
        juce::NormalisableRange<float>(0.001f, 5.0f, 0.001f, 0.4f), 0.01f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("filterDecay", 1), "Filter Decay",
        juce::NormalisableRange<float>(0.001f, 5.0f, 0.001f, 0.4f), 0.2f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("filterSustain", 1), "Filter Sustain", 0.0f, 1.0f, 0.7f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("filterRelease", 1), "Filter Release",
        juce::NormalisableRange<float>(0.001f, 5.0f, 0.001f, 0.4f), 0.3f));

    // Amp ADSR
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("ampAttack", 1), "Amp Attack",
        juce::NormalisableRange<float>(0.001f, 5.0f, 0.001f, 0.4f), 0.01f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("ampDecay", 1), "Amp Decay",
        juce::NormalisableRange<float>(0.001f, 5.0f, 0.001f, 0.4f), 0.2f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("ampSustain", 1), "Amp Sustain", 0.0f, 1.0f, 0.7f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("ampRelease", 1), "Amp Release",
        juce::NormalisableRange<float>(0.001f, 5.0f, 0.001f, 0.4f), 0.3f));

    // Chorus
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("chorusMode", 1), "Chorus",
        juce::StringArray{"Off", "Chorus I", "Chorus II", "I + II"}, 0));

    // Master
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("masterGain", 1), "Master",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.45f));

    // Mode
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("synthMode", 1), "Mode",
        juce::StringArray{"HARD BELGIAN MODE", "SAD ITALIAN MODE"}, 1));

    // LFO
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("lfoRate", 1), "LFO Rate",
        juce::NormalisableRange<float>(0.05f, 20.0f, 0.01f, 0.4f), 1.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("lfoDepth", 1), "LFO Depth", 0.0f, 1.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("lfoWaveform", 1), "LFO Wave",
        juce::StringArray{"Sine", "Triangle", "Saw", "Square", "S&H"}, 0));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("lfoDestination", 1), "LFO Dest",
        juce::StringArray{"Cutoff", "Pulse Width", "Pitch", "Volume"}, 0));

    // LFO Sync
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("lfoSync", 1), "LFO Sync",
        juce::StringArray{"Free", "Sync"}, 0));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("lfoSyncRate", 1), "LFO Sync Rate",
        juce::StringArray{"4/1", "2/1", "1/1", "1/2", "1/4", "1/8", "1/16", "1/32",
                          "1/4T", "1/8T", "1/16T", "1/2.", "1/4.", "1/8."}, 5)); // default 1/8

    // Triangle
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("triangleLevel", 1), "Triangle Level", 0.0f, 1.0f, 0.0f));

    // Noise
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("noiseLevel", 1), "Noise Level", 0.0f, 1.0f, 0.0f));

    // OSC2
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("osc2Saw", 1), "OSC2 Saw", 0.0f, 1.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("osc2Pulse", 1), "OSC2 Pulse", 0.0f, 1.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("osc2Triangle", 1), "OSC2 Triangle", 0.0f, 1.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("osc2PW", 1), "OSC2 PW", 0.05f, 0.95f, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("osc2Octave", 1), "OSC2 Octave",
        juce::StringArray{"-2", "-1", "0", "+1", "+2"}, 2));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("osc2Fine", 1), "OSC2 Fine",
        juce::NormalisableRange<float>(-100.0f, 100.0f, 0.1f), 0.0f));

    // Unison
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("unisonMode", 1), "Unison",
        juce::StringArray{"Off", "On"}, 0));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("unisonDetune", 1), "Detune",
        juce::NormalisableRange<float>(0.0f, 50.0f, 0.1f), 15.0f));

    // Key Tracking
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("keyTracking", 1), "Key Track", 0.0f, 1.0f, 0.0f));

    // Portamento / Glide
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("portamento", 1), "Portamento",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f, 0.5f), 0.0f));

    // Sequencer params
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("seqSwing", 1), "Seq Swing", 0.0f, 0.7f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("seqGateLength", 1), "Seq Gate",
        juce::NormalisableRange<float>(0.05f, 1.0f, 0.01f), 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID("seqNumSteps", 1), "Seq Steps", 1, 16, 16));

    // Oscillator volumes
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("osc1Volume", 1), "OSC1 Volume", 0.0f, 1.0f, 1.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("osc2Volume", 1), "OSC2 Volume", 0.0f, 1.0f, 1.0f));

    // Arp rate division (stepped)
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("arpRate", 1), "Arp Rate",
        juce::StringArray{"1/1", "1/2", "1/4", "1/8", "1/16", "1/32", "1/4T", "1/8T", "1/16T"}, 3)); // default 1/8

    // Arpeggiator pattern mode (UI exposure of an engine feature that already existed)
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("arpMode", 1), "Arp Mode",
        juce::StringArray{"Up", "Down", "Up-Down", "Random"}, 0)); // default Up

    // Global fine tune (cents)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("globalFineTune", 1), "Fine Tune",
        juce::NormalisableRange<float>(-100.0f, 100.0f, 0.1f), 0.0f));

    // (P37: Crush/LoFi parameters removed — DSP module deleted, replaced by
    // a stronger Sat stage in VintageProcessor.)

    // Arp gate length
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("arpGateLength", 1), "Arp Gate",
        juce::NormalisableRange<float>(0.05f, 1.0f, 0.01f), 0.5f));

    // Arp octave range (1-4)
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("arpOctaveRange", 1), "Arp Octaves",
        juce::StringArray{"1 Oct", "2 Oct", "3 Oct", "4 Oct"}, 0));

    // Sequencer rate division
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("seqRate", 1), "Seq Rate",
        juce::StringArray{"1/1", "1/2", "1/4", "1/8", "1/16", "1/32", "1/4T", "1/8T", "1/16T"}, 4)); // default 1/16

    // Stereo spread (0 = mono, 1 = max spread)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("stereoSpread", 1), "Stereo Spread",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

    // Velocity curve (0 = linear, 1 = exponential)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("velocityCurve", 1), "Velocity Curve",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.4f));

    // Vintage processor (post-chorus mastering chain)
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("vintageMode", 1), "Vintage",
        juce::StringArray{"Off", "On"}, 0));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("vintageAmount", 1), "Vintage Amount",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));

    return { params.begin(), params.end() };
}

void TsyganatorProcessor::prepareToPlay(double sampleRate, int /*samplesPerBlock*/)
{
    for (auto& v : voices)
        v.setSampleRate(sampleRate);
    chorus.setSampleRate(sampleRate);
    chorus.reset();
    lfo.setSampleRate(sampleRate);
    lfo.reset();
    sequencer.setSampleRate(sampleRate);
    arpeggiator.setSampleRate(sampleRate);
    samplePlayer.setHostSampleRate(sampleRate);
    vintage.setSampleRate(sampleRate);
    vintage.reset();
}

void TsyganatorProcessor::killAllNotes()
{
    for (auto& v : voices)
        v.noteOff();
    samplePlayer.stop();
}

void TsyganatorProcessor::setPlayMode(PlayMode newMode)
{
    if (newMode == playMode) return;

    // Kill everything when switching modes
    killAllNotes();

    if (isSequencerActive())
        sequencer.setPlaying(false);
    if (isArpActive())
        arpeggiator.allNotesOff();

    playMode = newMode;
}

void TsyganatorProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    buffer.clear();

    int numSamples = buffer.getNumSamples();
    if (numSamples <= 0) return;

    updateVoiceParams();
    float* outL = buffer.getWritePointer(0);
    float* outR = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : nullptr;

    // Chorus mode — getRawParameterValue returns the DENORMALIZED value (index) for Choice params
    int chorusIdx = juce::roundToInt(params.chorusMode->load());
    chorus.setMode(static_cast<JunoChorus::Mode>(std::clamp(chorusIdx, 0, 3)));

    float masterGain = params.masterGain->load();

    // Vintage processor parameters
    bool vintageOn = juce::roundToInt(params.vintageMode->load()) == 1;
    vintage.setEnabled(vintageOn);
    vintage.setAmount(params.vintageAmount->load());

    // LFO parameters — same: raw value IS the index for Choice params
    lfo.setDepth(params.lfoDepth->load());
    lfo.setWaveform(juce::roundToInt(params.lfoWaveform->load()));
    lfo.setDestination(juce::roundToInt(params.lfoDestination->load()));

    // LFO rate: free Hz or synced to host tempo
    bool lfoSyncOn = juce::roundToInt(params.lfoSync->load()) == 1;
    if (lfoSyncOn)
    {
        // Get BPM from host (fallback 120)
        double lfoBpm = 120.0;
        if (auto* ph = getPlayHead())
        {
            auto pos = ph->getPosition();
            if (pos.hasValue())
                lfoBpm = pos->getBpm().orFallback(120.0);
        }
        int syncIdx = juce::roundToInt(params.lfoSyncRate->load());
        lfo.setSyncRate(syncIdx, lfoBpm);
    }
    else
    {
        lfo.setRate(params.lfoRate->load());
    }

    // Sequencer parameters
    sequencer.setSwing(params.seqSwing->load());
    sequencer.setGateLength(params.seqGateLength->load());
    sequencer.setNumSteps(juce::roundToInt(params.seqNumSteps->load()));

    // Arpeggiator rate division — raw value IS the index
    arpeggiator.setRateDiv(juce::roundToInt(params.arpRate->load()));
    arpeggiator.setGateLength(params.arpGateLength->load());
    arpeggiator.setOctaveRange(juce::roundToInt(params.arpOctaveRange->load()) + 1); // 0-3 → 1-4
    // P21 (rebrand batch): wire the new arpMode parameter — engine already
    // supported the 4 modes, we just had no UI/automation hook before.
    arpeggiator.setMode(static_cast<Arpeggiator::Mode>(
        juce::roundToInt(params.arpMode->load())));

    // Sequencer rate division
    sequencer.setRateDiv(juce::roundToInt(params.seqRate->load()));

    // Sync sequencer/arp to host transport
    if (auto* playHead = getPlayHead())
    {
        auto posInfo = playHead->getPosition();
        if (posInfo.hasValue())
        {
            double bpm = posInfo->getBpm().orFallback(120.0);
            double ppq = posInfo->getPpqPosition().orFallback(0.0);
            bool hostPlaying = posInfo->getIsPlaying();

            if (isSequencerActive())
                sequencer.syncToHost(ppq, bpm, hostPlaying);
            if (isArpActive())
                arpeggiator.syncToHost(ppq, bpm, hostPlaying);

            // One-shot: only kill notes on the transition from playing→stopped
            // (not every block while stopped — that was Bug 3)
            if (!hostPlaying && wasHostPlaying)
            {
                if (isSequencerActive() || isArpActive())
                    killAllNotes();
            }
            wasHostPlaying = hostPlaying;
        }
    }

    // Clear pending MIDI output BEFORE processing input (so pass-through events survive)
    pendingMidiOut.clear();

    // P1-1 fix: pass-through unhandled messages (sysex, program change, channel
    // pressure, polyphonic aftertouch, song position, etc) so downstream
    // plugins still see them. The previous code did midiMessages.clear() at
    // the end which silently dropped everything we didn't explicitly handle.
    // Strategy: build pendingMidiOut as the FULL output buffer here, then swap
    // it into midiMessages at the end of processBlock.
    auto passthroughIfHandledNoteOrCC = [&](const juce::MidiMessage& msg, int samplePos)
    {
        // We add note-on/off to pendingMidiOut explicitly per play-mode below.
        // Other messages we already consumed (pitch bend, CC1, CC123) are intentionally
        // NOT echoed (we re-emit semantics ourselves via voices); everything else
        // we haven't touched should pass through.
        bool consumedByUs = msg.isPitchWheel()
                         || (msg.isController() && (msg.getControllerNumber() == 1
                                                  || msg.getControllerNumber() == 123));
        bool willEchoExplicitly = msg.isNoteOn() || msg.isNoteOff();
        if (!consumedByUs && !willEchoExplicitly)
            pendingMidiOut.addEvent(msg, samplePos);
    };

    // Process MIDI based on play mode
    for (const auto metadata : midiMessages)
    {
        auto msg = metadata.getMessage();

        // Pitch bend and CC messages apply globally regardless of play mode
        if (msg.isPitchWheel())
        {
            int pbValue = msg.getPitchWheelValue();
            pitchBendSemitones = ((float)pbValue - 8192.0f) / 8192.0f * 2.0f;
            // Echo pitch bend to host so a recording DAW captures it
            pendingMidiOut.addEvent(msg, metadata.samplePosition);
            continue;
        }
        if (msg.isController())
        {
            int cc = msg.getControllerNumber();
            if (cc == 1)  { modWheelValue = msg.getControllerValue() / 127.0f;
                            pendingMidiOut.addEvent(msg, metadata.samplePosition); continue; }
            if (cc == 123){ killAllNotes(); pitchBendSemitones = 0.0f; modWheelValue = 0.0f;
                            pendingMidiOut.addEvent(msg, metadata.samplePosition); continue; }
        }

        // Pass through anything we don't explicitly handle (sysex, PC, aftertouch…)
        passthroughIfHandledNoteOrCC(msg, metadata.samplePosition);

        switch (playMode)
        {
            case ModeOff:
                // Normal polyphonic play — keyboard → synth
                handleMidiMessage(msg);
                // Pass-through: echo MIDI to output so DAW sees note activity
                if (msg.isNoteOn() || msg.isNoteOff())
                    pendingMidiOut.addEvent(msg, metadata.samplePosition);
                break;

            case ModeArp:
                // Keyboard feeds arpeggiator → arp plays synth
                if (msg.isNoteOn())
                    arpeggiator.notePressed(msg.getNoteNumber(), msg.getFloatVelocity());
                else if (msg.isNoteOff())
                    arpeggiator.noteReleased(msg.getNoteNumber());
                else if (msg.isAllNotesOff() || msg.isAllSoundOff())
                    arpeggiator.allNotesOff();
                break;

            case ModeSeqSynth:
                // 303-style: keyboard sets transpose offset for sequencer
                if (msg.isNoteOn())
                {
                    int oldOffset = seqTransposeOffset.load(std::memory_order_relaxed);
                    seqTransposeHeldNote = msg.getNoteNumber();
                    int newOffset = msg.getNoteNumber() - seqBaseNote;
                    seqTransposeOffset.store(newOffset, std::memory_order_relaxed);

                    // Immediately retrigger current note with new transposition
                    // so the user hears the change without waiting for next step
                    int rawNote = sequencer.getLastPlayingNote();
                    if (sequencer.isPlaying() && rawNote >= 0)
                    {
                        int oldNote = std::clamp(rawNote + oldOffset, 0, 127);
                        int newNote = std::clamp(rawNote + newOffset, 0, 127);
                        handleSequencerNoteOff(oldNote);
                        handleSequencerNoteOn(newNote, 0.8f);
                    }
                }
                else if (msg.isNoteOff())
                {
                    // Only reset if releasing the currently held transpose note
                    if (msg.getNoteNumber() == seqTransposeHeldNote)
                    {
                        int oldOffset = seqTransposeOffset.load(std::memory_order_relaxed);
                        seqTransposeOffset.store(0, std::memory_order_relaxed);
                        seqTransposeHeldNote = -1;

                        // Retrigger at original pitch
                        int rawNote = sequencer.getLastPlayingNote();
                        if (sequencer.isPlaying() && rawNote >= 0)
                        {
                            int oldNote = std::clamp(rawNote + oldOffset, 0, 127);
                            int newNote = std::clamp(rawNote, 0, 127);
                            handleSequencerNoteOff(oldNote);
                            handleSequencerNoteOn(newNote, 0.8f);
                        }
                    }
                }
                break;

            case ModeSeqSample:
                // Sequencer drives sample — keyboard plays synth
                handleMidiMessage(msg);
                // Pass-through keyboard notes to MIDI output
                if (msg.isNoteOn() || msg.isNoteOff())
                    pendingMidiOut.addEvent(msg, metadata.samplePosition);
                break;
        }
    }

    // Clear incoming MIDI — we've processed all messages above, and what
    // should appear on output is now staged inside pendingMidiOut.
    // (Sequencer/arp callbacks in the sample loop below will also write
    // additional events into pendingMidiOut.)
    midiMessages.clear();

    // P1-6: re-apply voice params now that pitchBend/modWheel have been
    // updated by the MIDI loop above. Previous code called updateVoiceParams()
    // before reading MIDI, so bend/mod changes were always one block late.
    // Calling it again here is cheap (simple atomic loads + voice setters)
    // and removes the audible ~1.5ms latency on fast bends.
    updateVoiceParams();

    // Check unison mode for voice normalization
    bool unisonOn = juce::roundToInt(params.unisonMode->load()) == 1;
    // Unison uses all 6 voices on one note — normalize to prevent clipping
    float unisonScale = unisonOn ? (1.0f / std::sqrt((float)NUM_VOICES)) : 1.0f;

    // Stereo spread — voice panning across stereo field
    float stereoSpread = params.stereoSpread->load();

    // Render audio sample by sample
    for (int i = 0; i < numSamples; ++i)
    {
        currentRenderSample = i;

        // Advance sequencer or arpeggiator
        if (isSequencerActive())
            sequencer.process();
        if (isArpActive())
            arpeggiator.process();

        // Advance LFO
        float lfoVal = lfo.process();
        float lfoCutoffMod  = lfo.getCutoffMod(lfoVal);
        float lfoPwMod      = lfo.getPulseWidthMod(lfoVal);
        float lfoPitchMod   = lfo.getPitchMod(lfoVal);
        float lfoVolMod     = lfo.getVolumeMod(lfoVal);

        float monoL = 0.0f, monoR = 0.0f;
        for (int vi = 0; vi < NUM_VOICES; ++vi)
        {
            auto& v = voices[vi];
            if (v.isActive())
            {
                v.applyLFOMod(lfoCutoffMod, lfoPwMod, lfoPitchMod);
                float sample = v.process() * lfoVolMod;

                // Stereo spread: pan each voice across the stereo field
                if (stereoSpread > 0.001f && NUM_VOICES > 1)
                {
                    // Voice 0 pans left, voice N-1 pans right
                    float pan = (float)vi / (float)(NUM_VOICES - 1); // 0..1
                    pan = 0.5f + (pan - 0.5f) * stereoSpread;       // narrow around center
                    float gainL = std::cos(pan * 1.5707963f);        // equal-power pan
                    float gainR = std::sin(pan * 1.5707963f);
                    monoL += sample * gainL;
                    monoR += sample * gainR;
                }
                else
                {
                    monoL += sample;
                    monoR += sample;
                }
            }
        }

        // Apply unison normalization
        monoL *= unisonScale;
        monoR *= unisonScale;

        // Mix in sample player
        float sampleOut = samplePlayer.process();
        monoL += sampleOut;
        monoR += sampleOut;

        // (P37: LoFi/Crush stage removed — signal now goes straight to chorus.)

        // P1-2: Chorus is mono-sum-in / stereo-out by design. To preserve the
        // pre-chorus stereo image, we sum to mono for the wet ensemble effect
        // and then mix the dry stereo back on top. This was previously broken:
        // the diff was injected at 0.25× while the dry stereo was already lost
        // into the mono sum. Now: clean dry/wet stereo blend.
        const float monoSum = (monoL + monoR) * 0.5f;
        float wetL, wetR;
        chorus.process(monoSum, wetL, wetR);

        // Dry stereo cross-fades with wet chorus (P37: no LoFi stage anymore)
        // based on stereoSpread: 0 = pure mono-summed-then-chorus,
        // 1 = max stereo where dry channels dominate.
        const float wetAmt = 1.0f - stereoSpread * 0.5f;  // 1.0 → 0.5
        const float dryAmt = stereoSpread * 0.5f;          // 0.0 → 0.5
        float left  = wetL * wetAmt + monoL * dryAmt;
        float right = wetR * wetAmt + monoR * dryAmt;

        // Vintage processor (drive, EQ, bass mono, compression)
        vintage.process(left, right);

        // Apply master gain
        left *= masterGain;
        right *= masterGain;

        // Soft clipper (tanh) — prevents hard digital clipping
        left = std::tanh(left);
        right = std::tanh(right);

        outL[i] = left;
        if (outR) outR[i] = right;
    }

    // Copy pending MIDI output to the host buffer for DAW recording
    midiMessages.addEvents(pendingMidiOut, 0, numSamples, 0);

    // Update peak level for UI (mascot reactivity)
    float peak = 0.0f;
    for (int i = 0; i < numSamples; ++i)
    {
        float absL = std::abs(outL[i]);
        if (absL > peak) peak = absL;
        if (outR)
        {
            float absR = std::abs(outR[i]);
            if (absR > peak) peak = absR;
        }
    }
    peakLevel.store(peak, std::memory_order_relaxed);
}

void TsyganatorProcessor::handleMidiMessage(const juce::MidiMessage& msg)
{
    // Note: Pitch bend, mod wheel, and CC123 are handled globally in processBlock
    // before the play-mode switch, so they work in all modes.
    if (msg.isNoteOn())
        handleSequencerNoteOn(msg.getNoteNumber(), msg.getFloatVelocity());
    else if (msg.isNoteOff())
        handleSequencerNoteOff(msg.getNoteNumber());
    else if (msg.isAllNotesOff() || msg.isAllSoundOff())
        killAllNotes();
}

void TsyganatorProcessor::handleSequencerNoteOn(int note, float velocity)
{
    // Record timestamp for LED blink (Feature 3)
    lastNoteOnTime.store(juce::Time::getMillisecondCounterHiRes(), std::memory_order_relaxed);

    // Apply velocity curve: mix between linear (0) and exponential (1)
    float velCurve = params.velocityCurve->load();
    if (velCurve > 0.01f)
    {
        float expVel = velocity * velocity; // exponential (quadratic)
        velocity = velocity * (1.0f - velCurve) + expVel * velCurve;
    }

    bool unisonOn = juce::roundToInt(params.unisonMode->load()) == 1;
    float detuneCents = params.unisonDetune->load();

    if (unisonOn)
    {
        for (int i = 0; i < NUM_VOICES; ++i)
        {
            float spread = 0.0f;
            if (NUM_VOICES > 1)
                spread = -detuneCents + (2.0f * detuneCents * i) / (NUM_VOICES - 1);
            voices[i].setDetuneOffset(spread);
            voices[i].setAgeStamp(++voiceAgeCounter);
            voices[i].noteOn(note, velocity);
        }
        lastPlayedNote = note;
        lastUnisonNote = note;
        return;
    }

    // Normal polyphonic mode
    for (auto& v : voices)
        v.setDetuneOffset(0.0f);

    JunoVoice* target = nullptr;

    // 1) Re-trigger any voice already playing this note
    for (auto& v : voices)
        if (v.isActive() && v.getCurrentNote() == note)
        { target = &v; break; }

    // 2) Otherwise grab the first idle voice
    if (!target)
        for (auto& v : voices)
            if (!v.isActive())
            { target = &v; break; }

    // 3) P2-5: Voice stealing — pick the OLDEST voice currently in release stage.
    //    Falls back to the oldest active voice if none are releasing.
    if (!target)
    {
        JunoVoice* oldestReleasing = nullptr;
        JunoVoice* oldestActive = nullptr;
        for (auto& v : voices)
        {
            if (v.isInRelease())
            {
                if (!oldestReleasing || v.getAgeStamp() < oldestReleasing->getAgeStamp())
                    oldestReleasing = &v;
            }
            if (!oldestActive || v.getAgeStamp() < oldestActive->getAgeStamp())
                oldestActive = &v;
        }
        target = oldestReleasing ? oldestReleasing : oldestActive;
        if (!target) target = &voices[voiceStealIndex++ % NUM_VOICES]; // ultra-safe fallback
    }

    target->setAgeStamp(++voiceAgeCounter);
    target->noteOn(note, velocity);
    lastPlayedNote = note;
}

void TsyganatorProcessor::handleSequencerNoteOff(int note)
{
    bool unisonOn = juce::roundToInt(params.unisonMode->load()) == 1;

    if (unisonOn)
    {
        // Only release if this is the note that's actually playing in unison
        if (note == lastUnisonNote)
        {
            for (auto& v : voices)
                v.noteOff();
            lastUnisonNote = -1;
        }
        return;
    }

    for (auto& v : voices)
        if (v.isActive() && v.getCurrentNote() == note)
            v.noteOff();
}

void TsyganatorProcessor::updateVoiceParams()
{
    // P2-1/P2-2: all reads go through cached atomic pointers (no string hashing).
    // Also hoisted triangleLevel + osc2 reads out of the per-voice loop.
    const float saw           = params.sawLevel->load();
    const float pulse         = params.pulseLevel->load();
    const float sub           = params.subLevel->load();
    const float noise         = params.noiseLevel->load();
    const float pw            = params.pulseWidth->load();
    const float triLevel      = params.triangleLevel->load();

    const float osc1Vol       = params.osc1Volume->load();
    const float osc2Vol       = params.osc2Volume->load();

    const float osc2Saw       = params.osc2Saw->load();
    const float osc2Pulse     = params.osc2Pulse->load();
    const float osc2Triangle  = params.osc2Triangle->load();
    const float osc2PW        = params.osc2PW->load();
    const int   osc2OctVal    = juce::roundToInt(params.osc2Octave->load()) - 2;
    const float osc2Fine      = params.osc2Fine->load();

    const float cutoff        = params.cutoff->load();
    const float reso          = params.resonance->load();
    const float fEnvAmt       = params.filterEnvAmount->load();
    const float keyTrack      = params.keyTracking->load();

    const float fA = params.filterAttack->load();
    const float fD = params.filterDecay->load();
    const float fS = params.filterSustain->load();
    const float fR = params.filterRelease->load();

    const float aA = params.ampAttack->load();
    const float aD = params.ampDecay->load();
    const float aS = params.ampSustain->load();
    const float aR = params.ampRelease->load();

    const float portamento    = params.portamento->load();
    const float globalFineTune = params.globalFineTune->load();

    // Mod wheel → cutoff boost (up to +4000 Hz)
    const float modWheelCutoffBoost = modWheelValue * 4000.0f;

    for (int i = 0; i < NUM_VOICES; ++i)
    {
        auto& v = voices[i];
        v.setGlobalFineTune(globalFineTune);
        v.setPitchBend(pitchBendSemitones);
        v.setSawLevel(saw * osc1Vol);
        v.setPulseLevel(pulse * osc1Vol);
        v.setTriangleLevel(triLevel * osc1Vol);
        v.setSubLevel(sub * osc1Vol);
        v.setNoiseLevel(noise * osc1Vol);
        v.setBasePulseWidth(pw);
        v.setPortamento(portamento);

        // OSC2
        v.setOsc2SawLevel(osc2Saw * osc2Vol);
        v.setOsc2PulseLevel(osc2Pulse * osc2Vol);
        v.setOsc2TriangleLevel(osc2Triangle * osc2Vol);
        v.setOsc2PulseWidth(osc2PW);
        v.setOsc2Octave(osc2OctVal);
        v.setOsc2Fine(osc2Fine);

        // Key tracking
        float trackOffset = 0.0f;
        if (keyTrack > 0.0f && v.isActive() && v.getCurrentNote() >= 0)
        {
            float semitones = (float)(v.getCurrentNote() - 60);
            trackOffset = semitones * (cutoff / 60.0f) * keyTrack;
        }
        v.setFilterCutoff(cutoff + trackOffset + modWheelCutoffBoost);
        v.setFilterResonance(reso);
        v.setFilterEnvAmount(fEnvAmt);
        v.setFilterADSR(fA, fD, fS, fR);
        v.setAmpADSR(aA, aD, aS, aR);
    }
}

// ============================================================
//  Tsyganize — MUSICAL randomization that PRESERVES the preset character.
//  Instead of replacing all values from scratch, it NUDGES current parameters
//  by a small amount. This ensures the preset remains recognizable while
//  adding a unique flavor — never "bouillie sonore" (sonic mush).
//
//  Thread safety: uses setValueNotifyingHost (RT-safe) instead of
//  juce::Value::setValue, so it's safe even if the host invokes us
//  off the message thread.
// ============================================================
void TsyganatorProcessor::tsyganize()
{
    auto randFloat = [this](float min, float max) {
        return std::uniform_real_distribution<float>(min, max)(rng);
    };
    auto randInt = [this](int min, int max) {
        return std::uniform_int_distribution<int>(min, max)(rng);
    };

    // Helper: set a parameter via the RT-safe APVTS path
    auto setParam = [this](const char* paramId, float value) {
        if (auto* p = apvts.getParameter(paramId))
            p->setValueNotifyingHost(p->convertTo0to1(value));
    };

    // Helper: nudge a parameter by ±percentage of its current value, clamped to [min,max]
    auto nudge = [&](const char* paramId, float pct, float lo, float hi) {
        float cur = apvts.getRawParameterValue(paramId)->load();
        float range = (hi - lo) * pct;
        float nudged = cur + randFloat(-range, range);
        setParam(paramId, juce::jlimit(lo, hi, nudged));
    };

    // === OSC1 — gentle mix adjustment, preserve the preset's character ===
    nudge("sawLevel",      0.10f, 0.0f, 1.0f);
    nudge("pulseLevel",    0.10f, 0.0f, 1.0f);
    nudge("triangleLevel", 0.10f, 0.0f, 1.0f);
    nudge("subLevel",      0.10f, 0.0f, 1.0f);
    nudge("noiseLevel",    0.08f, 0.0f, 1.0f);

    // === OSC2 — subtle adjustments, preserve the mix balance ===
    nudge("osc2Saw",      0.15f, 0.0f, 1.0f);
    nudge("osc2Pulse",    0.15f, 0.0f, 1.0f);
    nudge("osc2Triangle", 0.15f, 0.0f, 0.8f);
    nudge("osc2PW",       0.12f, 0.1f, 0.9f);
    nudge("osc2Fine",     0.10f, -50.0f, 50.0f);
    // OSC2 octave: keep same (changing octave destroys the preset character)

    // === Filter — nudge gently, never kill the sound ===
    nudge("cutoff",         0.15f, 100.0f, 16000.0f);
    nudge("resonance",      0.10f, 0.0f, 0.5f);
    nudge("filterEnvAmount",0.15f, 0.0f, 1.0f);

    // === Filter ADSR — very small nudges to preserve envelope shape ===
    nudge("filterAttack",   0.10f, 0.001f, 0.5f);
    nudge("filterDecay",    0.10f, 0.02f, 0.8f);
    nudge("filterSustain",  0.10f, 0.0f, 1.0f);
    nudge("filterRelease",  0.10f, 0.02f, 1.0f);

    // === Amp ADSR — tiny nudges only, never make attack sluggish ===
    nudge("ampAttack",      0.08f, 0.001f, 0.2f);
    nudge("ampDecay",       0.08f, 0.02f, 0.6f);
    nudge("ampSustain",     0.08f, 0.0f, 1.0f);
    nudge("ampRelease",     0.08f, 0.02f, 0.8f);

    // === LFO — gentle nudge on rate/depth, optionally change wave/dest ===
    nudge("lfoRate",  0.15f, 0.1f, 12.0f);
    nudge("lfoDepth", 0.12f, 0.0f, 0.5f);
    // 30% chance to change LFO waveform or destination
    if (randFloat(0.0f, 1.0f) < 0.3f)
        setParam("lfoWaveform", (float)randInt(0, 4));
    if (randFloat(0.0f, 1.0f) < 0.3f)
        setParam("lfoDestination", (float)randInt(0, 3));

    // === Unison — 25% chance to toggle (adds or removes thickness) ===
    if (randFloat(0.0f, 1.0f) < 0.25f)
        setParam("unisonMode", (float)randInt(0, 1));
    nudge("unisonDetune",  0.12f, 0.0f, 40.0f);

    // === Effects — light touch ===
    // 25% chance to change chorus mode
    if (randFloat(0.0f, 1.0f) < 0.25f)
        setParam("chorusMode", (float)randInt(0, 3));
    // 30% chance to toggle Vintage
    if (randFloat(0.0f, 1.0f) < 0.3f)
        setParam("vintageMode", (float)randInt(0, 1));
    nudge("vintageAmount", 0.15f, 0.2f, 0.8f);

    // === Pulse width (OSC1) — small wobble ===
    nudge("pulseWidth", 0.10f, 0.1f, 0.9f);

    // === Performance — conservative nudges ===
    nudge("portamento",    0.08f, 0.0f, 0.3f);
    // Fine tune: very subtle (±3 cents max) — prevents pitch drift
    setParam("globalFineTune", randFloat(-3.0f, 3.0f));
}

void TsyganatorProcessor::setMode(SynthMode newMode)
{
    if (newMode == currentMode) return;
    currentMode = newMode;
    currentModePresets = (currentMode == BelgianMode) ? getBelgianPresets() : getItalianPresets();
    loadPreset(0);

    for (auto* l : modeListeners)
        l->modeChanged(currentMode);
}

void TsyganatorProcessor::loadPreset(int index)
{
    if (index < 0 || index >= (int)currentModePresets.size()) return;
    currentPreset = index;
    const auto& p = currentModePresets[(size_t)index];

    // RT-safe parameter setter: uses APVTS's atomic path with proper host notification.
    // Safe to call from message thread OR audio thread (e.g. host program change automation).
    auto setP = [this](const char* id, float value) {
        if (auto* param = apvts.getParameter(id))
            param->setValueNotifyingHost(param->convertTo0to1(value));
    };

    setP("sawLevel",        p.sawLevel);
    setP("pulseLevel",      p.pulseLevel);
    setP("triangleLevel",   p.triangleLevel);
    setP("subLevel",        p.subLevel);
    setP("noiseLevel",      p.noiseLevel);
    setP("pulseWidth",      p.pulseWidth);
    setP("osc2Saw",         p.osc2Saw);
    setP("osc2Pulse",       p.osc2Pulse);
    setP("osc2Triangle",    p.osc2Triangle);
    setP("osc2PW",          p.osc2PW);
    setP("osc2Octave",      (float)(p.osc2Octave + 2));
    setP("osc2Fine",        p.osc2Fine);
    setP("cutoff",          p.cutoff);
    setP("resonance",       p.resonance);
    setP("filterEnvAmount", p.filterEnvAmount);
    setP("filterAttack",    p.filterAttack);
    setP("filterDecay",     p.filterDecay);
    setP("filterSustain",   p.filterSustain);
    setP("filterRelease",   p.filterRelease);
    setP("ampAttack",       p.ampAttack);
    setP("ampDecay",        p.ampDecay);
    setP("ampSustain",      p.ampSustain);
    setP("ampRelease",      p.ampRelease);
    setP("chorusMode",      (float)p.chorusMode);
    setP("masterGain",      p.masterGain);
    setP("lfoRate",         p.lfoRate);
    setP("lfoDepth",        p.lfoDepth);
    setP("lfoWaveform",     (float)p.lfoWaveform);
    setP("lfoDestination",  (float)p.lfoDestination);
    setP("keyTracking",     p.keyTracking);
    setP("portamento",      p.portamento);
    // Reset fine tune to 0 on preset load — prevents inherited Tsyganize detuning
    setP("globalFineTune",  0.0f);

    // Reset oscillator master volumes — presets use per-waveform levels for mixing
    setP("osc1Volume",      1.0f);
    setP("osc2Volume",      1.0f);

    // Load vintage settings from preset (or default off)
    setP("vintageMode",     p.vintageMode ? 1.0f : 0.0f);
    setP("vintageAmount",   p.vintageAmount);
    vintage.reset();

    // Reset LFO phase so new preset's LFO starts fresh (no awkward transition)
    lfo.reset();

    // Unison always OFF on preset load — user activates manually if desired
    setP("unisonMode",      0.0f);
    setP("unisonDetune",    p.unisonDetune);
}

void TsyganatorProcessor::setCurrentProgram(int index) { loadPreset(index); }

const juce::String TsyganatorProcessor::getProgramName(int index)
{
    if (index >= 0 && index < (int)currentModePresets.size())
        return juce::String(currentModePresets[(size_t)index].name);
    return {};
}

juce::AudioProcessorEditor* TsyganatorProcessor::createEditor()
{
    return new TsyganatorEditor(*this);
}

void TsyganatorProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty("synthModeValue", (int)currentMode, nullptr);
    state.setProperty("playModeValue", (int)playMode, nullptr);
    state.setProperty("arpMode", (int)arpeggiator.getMode(), nullptr);
    state.setProperty("currentPresetIndex", currentPreset, nullptr);

    // Save sequencer pattern
    juce::ValueTree seqState("SequencerPattern");
    seqState.setProperty("numSteps", sequencer.getNumSteps(), nullptr);
    seqState.setProperty("swing", (double)sequencer.getSwing(), nullptr);
    seqState.setProperty("gateLength", (double)sequencer.getGateLength(), nullptr);

    for (int i = 0; i < StepSequencer::MAX_STEPS; ++i)
    {
        juce::ValueTree stepState("Step");
        const auto& step = sequencer.getStep(i);
        stepState.setProperty("note", step.note, nullptr);
        stepState.setProperty("velocity", (double)step.velocity, nullptr);
        stepState.setProperty("active", step.active, nullptr);
        stepState.setProperty("glide", step.glide, nullptr);
        stepState.setProperty("accent", step.accent, nullptr);
        seqState.addChild(stepState, -1, nullptr);
    }
    state.addChild(seqState, -1, nullptr);

    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void TsyganatorProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml && xml->hasTagName(apvts.state.getType()))
    {
        apvts.replaceState(juce::ValueTree::fromXml(*xml));

        // Restore mode WITHOUT calling loadPreset(0) — parameters are already restored by replaceState
        int modeVal = apvts.state.getProperty("synthModeValue", 0);
        currentMode = static_cast<SynthMode>(std::clamp(modeVal, 0, 1));
        currentModePresets = (currentMode == BelgianMode) ? getBelgianPresets() : getItalianPresets();
        currentPreset = (int)apvts.state.getProperty("currentPresetIndex", 0);
        // Notify mode listeners
        for (auto* l : modeListeners)
            l->modeChanged(currentMode);

        int pmVal = apvts.state.getProperty("playModeValue", 0);
        playMode = static_cast<PlayMode>(std::clamp(pmVal, 0, 3));

        arpeggiator.setMode(static_cast<Arpeggiator::Mode>(
            (int)apvts.state.getProperty("arpMode", 0)));

        // Restore sequencer
        auto seqState = apvts.state.getChildWithName("SequencerPattern");
        if (seqState.isValid())
        {
            sequencer.setNumSteps((int)seqState.getProperty("numSteps", 16));
            sequencer.setSwing((float)(double)seqState.getProperty("swing", 0.0));
            sequencer.setGateLength((float)(double)seqState.getProperty("gateLength", 0.5));

            for (int i = 0; i < seqState.getNumChildren() && i < StepSequencer::MAX_STEPS; ++i)
            {
                auto stepState = seqState.getChild(i);
                sequencer.setStepNote(i, (int)stepState.getProperty("note", 60));
                sequencer.setStepVelocity(i, (float)(double)stepState.getProperty("velocity", 0.8));
                sequencer.setStepActive(i, (bool)stepState.getProperty("active", false));
                sequencer.setStepGlide(i, (bool)stepState.getProperty("glide", false));
                sequencer.setStepAccent(i, (bool)stepState.getProperty("accent", false));
            }
        }
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new TsyganatorProcessor();
}
