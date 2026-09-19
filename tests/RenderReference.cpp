/*
  ============================================================================
   Tsyganator — Offline render-reference harness
  ============================================================================
   Renders fixed, fully-specified scenarios through TsyganatorProcessor and
   writes deterministic output, so any DSP change can be proven bit-exact
   (or measured precisely when it is intentionally audible).

   DETERMINISM CONTRACT
   --------------------
   The engine seeds several RNGs from std::random_device, so these paths are
   NOT reproducible and are deliberately avoided by every scenario below:
       * noiseLevel > 0          (JunoOscillator::noiseGen)
       * LFO waveform = S&H      (JunoLFO::rng)
       * Arp mode = Random       (Arpeggiator::rng)
       * randomize() / tsyganize()
   Scenarios set every parameter they depend on explicitly, so they do not
   drift when preset data or parameter defaults change.

   Output per scenario:
       <name>.f32  interleaved little-endian float32 — the comparison artifact
       <name>.wav  24-bit WAV — for listening

   Usage:
       tsyg_render --list
       tsyg_render --render <outDir>
       tsyg_render --compare <a.f32> <b.f32>
  ============================================================================
*/

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include "PluginProcessor.h"

#include <functional>
#include <vector>

//==============================================================================
namespace
{

struct MidiEvent
{
    int samplePos;
    juce::MidiMessage msg;
};

struct Scenario
{
    juce::String name;
    juce::String description;
    double       sampleRate;
    int          blockSize;
    double       seconds;
    std::function<void (TsyganatorProcessor&)>          setup;
    std::function<std::vector<MidiEvent> (double)>      midi;
};

//==============================================================================
void setParam (TsyganatorProcessor& p, const char* id, float value)
{
    if (auto* prm = p.apvts.getParameter (id))
        prm->setValueNotifyingHost (prm->convertTo0to1 (value));
    else
        std::cerr << "WARNING: unknown parameter id '" << id << "'\n";
}

// Silence every sound source, then each scenario opts back in to what it needs.
// Keeps scenarios independent of parameter defaults.
void neutralBase (TsyganatorProcessor& p)
{
    p.setPlayMode (TsyganatorProcessor::ModeOff);   // plain synth: MIDI in, no seq/arp

    setParam (p, "sawLevel", 0.0f);      setParam (p, "pulseLevel", 0.0f);
    setParam (p, "subLevel", 0.0f);      setParam (p, "triangleLevel", 0.0f);
    setParam (p, "noiseLevel", 0.0f);                       // MUST stay 0 (RNG)
    setParam (p, "osc2Saw", 0.0f);       setParam (p, "osc2Pulse", 0.0f);
    setParam (p, "osc2Triangle", 0.0f);
    setParam (p, "osc1Volume", 0.8f);    setParam (p, "osc2Volume", 0.8f);
    setParam (p, "osc2Octave", 0.0f);    setParam (p, "osc2Fine", 0.0f);
    setParam (p, "pulseWidth", 0.5f);    setParam (p, "osc2PW", 0.5f);

    setParam (p, "cutoff", 12000.0f);    setParam (p, "resonance", 0.0f);
    setParam (p, "filterEnvAmount", 0.0f);
    setParam (p, "filterAttack", 0.01f); setParam (p, "filterDecay", 0.2f);
    setParam (p, "filterSustain", 1.0f); setParam (p, "filterRelease", 0.2f);

    setParam (p, "ampAttack", 0.005f);   setParam (p, "ampDecay", 0.2f);
    setParam (p, "ampSustain", 0.8f);    setParam (p, "ampRelease", 0.3f);

    setParam (p, "lfoDepth", 0.0f);      setParam (p, "lfoRate", 1.0f);
    setParam (p, "lfoWaveform", 0.0f);                      // Sine, never S&H
    setParam (p, "lfoDestination", 0.0f);
    setParam (p, "lfoSync", 0.0f);

    setParam (p, "chorusMode", 0.0f);    setParam (p, "vintageMode", 0.0f);
    setParam (p, "vintageAmount", 0.0f);
    setParam (p, "unisonMode", 0.0f);    setParam (p, "unisonDetune", 0.0f);
    setParam (p, "stereoSpread", 0.0f);  setParam (p, "portamento", 0.0f);
    setParam (p, "keyTracking", 0.0f);   setParam (p, "globalFineTune", 0.0f);
    setParam (p, "velocityCurve", 0.5f);
    setParam (p, "masterGain", 0.7f);
}

int secToSamp (double t, double sr) { return (int) (t * sr); }

//==============================================================================
std::vector<Scenario> buildScenarios()
{
    std::vector<Scenario> s;

    // ---- 1. Polyphonic saw + sub through the filter envelope and chorus ----
    s.push_back ({
        "poly_saw_filter",
        "4-voice chord + melody: dual osc, sub, filter env, amp env, Chorus II",
        48000.0, 512, 4.0,
        [] (TsyganatorProcessor& p)
        {
            neutralBase (p);
            setParam (p, "sawLevel", 1.0f);
            setParam (p, "subLevel", 0.35f);
            setParam (p, "osc2Saw", 0.6f);
            setParam (p, "osc2Octave", -1.0f);
            setParam (p, "osc2Fine", 7.0f);
            setParam (p, "cutoff", 1200.0f);
            setParam (p, "resonance", 0.4f);
            setParam (p, "filterEnvAmount", 0.6f);
            setParam (p, "filterAttack", 0.01f);
            setParam (p, "filterDecay", 0.35f);
            setParam (p, "filterSustain", 0.35f);
            setParam (p, "ampAttack", 0.005f);
            setParam (p, "ampDecay", 0.25f);
            setParam (p, "ampSustain", 0.7f);
            setParam (p, "ampRelease", 0.4f);
            setParam (p, "chorusMode", 2.0f);          // Chorus II
        },
        [] (double sr)
        {
            std::vector<MidiEvent> e;
            const int chord[] = { 48, 52, 55, 59 };    // C3 E3 G3 B3
            for (int n : chord)
                e.push_back ({ secToSamp (0.10, sr), juce::MidiMessage::noteOn (1, n, 0.80f) });
            for (int n : chord)
                e.push_back ({ secToSamp (1.60, sr), juce::MidiMessage::noteOff (1, n) });

            // Melody on top, exercises voice allocation + release overlap
            const int mel[]  = { 67, 70, 72, 75 };
            for (int i = 0; i < 4; ++i)
            {
                double t = 2.0 + i * 0.35;
                e.push_back ({ secToSamp (t,        sr), juce::MidiMessage::noteOn  (1, mel[i], 0.9f) });
                e.push_back ({ secToSamp (t + 0.28, sr), juce::MidiMessage::noteOff (1, mel[i]) });
            }
            return e;
        }
    });

    // ---- 2. Mono lead: LFO→cutoff, portamento glide, pitch bend ----
    //  This scenario is the witness for the portamento / LFO-pitch interaction.
    s.push_back ({
        "mono_lead_lfo",
        "Legato lead: LFO to cutoff, portamento glide, pitch bend",
        48000.0, 256, 4.0,
        [] (TsyganatorProcessor& p)
        {
            neutralBase (p);
            setParam (p, "pulseLevel", 1.0f);
            setParam (p, "pulseWidth", 0.35f);
            setParam (p, "subLevel", 0.25f);
            setParam (p, "cutoff", 900.0f);
            setParam (p, "resonance", 0.62f);
            setParam (p, "filterEnvAmount", 0.35f);
            setParam (p, "lfoDepth", 0.5f);
            setParam (p, "lfoRate", 3.5f);
            setParam (p, "lfoWaveform", 0.0f);          // Sine
            setParam (p, "lfoDestination", 0.0f);       // Cutoff
            setParam (p, "portamento", 0.30f);
            setParam (p, "ampRelease", 0.5f);
        },
        [] (double sr)
        {
            std::vector<MidiEvent> e;
            // Legato chain -> triggers portamento between notes
            const int notes[] = { 40, 47, 43, 52, 45 };
            for (int i = 0; i < 5; ++i)
            {
                double t = 0.10 + i * 0.55;
                e.push_back ({ secToSamp (t, sr), juce::MidiMessage::noteOn (1, notes[i], 0.85f) });
                if (i > 0)
                    e.push_back ({ secToSamp (t + 0.02, sr), juce::MidiMessage::noteOff (1, notes[i - 1]) });
            }
            e.push_back ({ secToSamp (2.90, sr), juce::MidiMessage::pitchWheel (1, 10000) });
            e.push_back ({ secToSamp (3.20, sr), juce::MidiMessage::pitchWheel (1, 8192) });
            e.push_back ({ secToSamp (3.40, sr), juce::MidiMessage::noteOff (1, 45) });
            return e;
        }
    });

    // ---- 3. Vintage chain + unison + stereo spread, at 44.1k ----
    s.push_back ({
        "vintage_unison_44k",
        "Vintage drive/EQ/comp + unison detune + stereo spread, 44.1 kHz",
        44100.0, 480, 3.5,
        [] (TsyganatorProcessor& p)
        {
            neutralBase (p);
            setParam (p, "sawLevel", 0.9f);
            setParam (p, "triangleLevel", 0.4f);
            setParam (p, "osc2Saw", 0.7f);
            setParam (p, "osc2Fine", -9.0f);
            setParam (p, "cutoff", 2400.0f);
            setParam (p, "resonance", 0.25f);
            setParam (p, "filterEnvAmount", 0.45f);
            setParam (p, "unisonMode", 1.0f);
            setParam (p, "unisonDetune", 0.40f);
            setParam (p, "stereoSpread", 0.80f);
            setParam (p, "chorusMode", 3.0f);           // I + II
            setParam (p, "vintageMode", 1.0f);
            setParam (p, "vintageAmount", 0.70f);
            setParam (p, "masterGain", 0.85f);
        },
        [] (double sr)
        {
            std::vector<MidiEvent> e;
            e.push_back ({ secToSamp (0.10, sr), juce::MidiMessage::noteOn  (1, 36, 1.00f) });
            e.push_back ({ secToSamp (1.20, sr), juce::MidiMessage::noteOff (1, 36) });
            e.push_back ({ secToSamp (1.30, sr), juce::MidiMessage::noteOn  (1, 43, 0.70f) });
            e.push_back ({ secToSamp (1.35, sr), juce::MidiMessage::noteOn  (1, 50, 0.55f) });
            e.push_back ({ secToSamp (2.60, sr), juce::MidiMessage::noteOff (1, 43) });
            e.push_back ({ secToSamp (2.60, sr), juce::MidiMessage::noteOff (1, 50) });
            return e;
        }
    });

    return s;
}

//==============================================================================
juce::AudioBuffer<float> renderScenario (const Scenario& sc)
{
    TsyganatorProcessor proc;

    proc.setPlayHead (nullptr);          // no transport: seq/arp stay silent
    proc.setNonRealtime (true);
    sc.setup (proc);
    proc.prepareToPlay (sc.sampleRate, sc.blockSize);

    auto events = sc.midi (sc.sampleRate);
    std::sort (events.begin(), events.end(),
               [] (const MidiEvent& a, const MidiEvent& b) { return a.samplePos < b.samplePos; });

    const int numBlocks = (int) std::ceil (sc.seconds * sc.sampleRate / sc.blockSize);
    const int total     = numBlocks * sc.blockSize;

    juce::AudioBuffer<float> out (2, total);
    out.clear();

    juce::AudioBuffer<float> block (2, sc.blockSize);
    size_t evIdx = 0;

    for (int b = 0; b < numBlocks; ++b)
    {
        const int pos = b * sc.blockSize;
        block.clear();

        juce::MidiBuffer mb;
        while (evIdx < events.size() && events[evIdx].samplePos < pos + sc.blockSize)
        {
            mb.addEvent (events[evIdx].msg,
                         juce::jmax (0, events[evIdx].samplePos - pos));
            ++evIdx;
        }

        proc.processBlock (block, mb);

        for (int ch = 0; ch < 2; ++ch)
            out.copyFrom (ch, pos, block, ch, 0, sc.blockSize);
    }

    return out;
}

//==============================================================================
bool writeRaw (const juce::File& f, const juce::AudioBuffer<float>& buf)
{
    f.deleteFile();
    juce::FileOutputStream os (f);
    if (os.failedToOpen()) return false;

    for (int i = 0; i < buf.getNumSamples(); ++i)
        for (int ch = 0; ch < buf.getNumChannels(); ++ch)
            os.writeFloat (buf.getSample (ch, i));   // little-endian float32

    return true;
}

bool writeWav (const juce::File& f, const juce::AudioBuffer<float>& buf, double sr)
{
    f.deleteFile();
    juce::WavAudioFormat wav;
    auto os = std::make_unique<juce::FileOutputStream> (f);
    if (os->failedToOpen()) return false;

    std::unique_ptr<juce::AudioFormatWriter> w (
        wav.createWriterFor (os.release(), sr, (unsigned int) buf.getNumChannels(), 24, {}, 0));
    if (w == nullptr) return false;

    return w->writeFromAudioSampleBuffer (buf, 0, buf.getNumSamples());
}

bool readRaw (const juce::File& f, std::vector<float>& out)
{
    juce::FileInputStream is (f);
    if (is.failedToOpen()) return false;
    out.clear();
    out.reserve ((size_t) (f.getSize() / 4));
    while (! is.isExhausted())
        out.push_back (is.readFloat());
    return true;
}

//==============================================================================
int doCompare (const juce::File& fa, const juce::File& fb)
{
    std::vector<float> a, b;
    if (! readRaw (fa, a)) { std::cerr << "cannot read " << fa.getFullPathName() << "\n"; return 2; }
    if (! readRaw (fb, b)) { std::cerr << "cannot read " << fb.getFullPathName() << "\n"; return 2; }

    if (a.size() != b.size())
    {
        std::cout << "DIFFERENT LENGTH: " << a.size() << " vs " << b.size() << " samples\n";
        return 1;
    }

    double maxAbs = 0.0, sumSq = 0.0, refSq = 0.0;
    long long firstDiff = -1, numDiff = 0;

    for (size_t i = 0; i < a.size(); ++i)
    {
        const double d = (double) a[i] - (double) b[i];
        if (d != 0.0)
        {
            ++numDiff;
            if (firstDiff < 0) firstDiff = (long long) i;
        }
        maxAbs = juce::jmax (maxAbs, std::abs (d));
        sumSq += d * d;
        refSq += (double) a[i] * (double) a[i];
    }

    if (numDiff == 0)
    {
        std::cout << "IDENTICAL (" << a.size() << " samples, bit-exact)\n";
        return 0;
    }

    const double rms    = std::sqrt (sumSq / (double) a.size());
    const double refRms = std::sqrt (refSq / (double) a.size());
    const double snr    = (rms > 0.0 && refRms > 0.0)
                            ? 20.0 * std::log10 (refRms / rms) : 0.0;

    std::cout << "DIFFERENT\n"
              << "  samples differing : " << numDiff << " / " << a.size()
              << "  (" << juce::String (100.0 * (double) numDiff / (double) a.size(), 2) << "%)\n"
              << "  first difference  : sample " << firstDiff << "\n"
              << "  max abs diff      : " << juce::String (maxAbs, 9)
              << "  (" << juce::String (juce::Decibels::gainToDecibels (maxAbs), 2) << " dBFS)\n"
              << "  rms diff          : " << juce::String (rms, 9) << "\n"
              << "  difference SNR    : " << juce::String (snr, 2) << " dB\n";
    return 1;
}

} // namespace

//==============================================================================
int main (int argc, char* argv[])
{
    juce::ScopedJuceInitialiser_GUI juceInit;

    juce::StringArray args;
    for (int i = 1; i < argc; ++i) args.add (juce::String (argv[i]));

    auto scenarios = buildScenarios();

    if (args.isEmpty() || args[0] == "--help")
    {
        std::cout << "tsyg_render --list\n"
                  << "tsyg_render --render <outDir>\n"
                  << "tsyg_render --compare <a.f32> <b.f32>\n";
        return 0;
    }

    if (args[0] == "--list")
    {
        for (auto& sc : scenarios)
            std::cout << sc.name << "  [" << sc.sampleRate << " Hz, block " << sc.blockSize
                      << ", " << sc.seconds << " s]\n    " << sc.description << "\n";
        return 0;
    }

    if (args[0] == "--compare")
    {
        if (args.size() < 3) { std::cerr << "--compare needs two files\n"; return 2; }
        return doCompare (juce::File (args[1]), juce::File (args[2]));
    }

    if (args[0] == "--render")
    {
        if (args.size() < 2) { std::cerr << "--render needs an output directory\n"; return 2; }
        juce::File outDir (args[1]);
        outDir.createDirectory();

        for (auto& sc : scenarios)
        {
            std::cout << "rendering " << sc.name << " ... " << std::flush;
            auto buf = renderScenario (sc);

            auto raw = outDir.getChildFile (sc.name + ".f32");
            auto wav = outDir.getChildFile (sc.name + ".wav");

            if (! writeRaw (raw, buf)) { std::cerr << "FAILED writing " << raw.getFullPathName() << "\n"; return 2; }
            writeWav (wav, buf, sc.sampleRate);

            std::cout << buf.getNumSamples() << " frames -> " << raw.getFileName() << "\n";
        }
        return 0;
    }

    std::cerr << "unknown option: " << args[0] << "\n";
    return 2;
}
