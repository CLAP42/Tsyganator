#pragma once
#include <cstdint>
#include "JunoOscillator.h"
#include "JunoFilter.h"
#include "JunoEnvelope.h"

/**
 * JunoVoice — Single voice of the Tsyganator synthesizer
 * Dual oscillator + filter + two ADSR envelopes (amp & filter).
 * OSC1: saw/pulse/sub/noise with PW control
 * OSC2: saw/pulse with octave offset, fine detune, own PW
 */
class JunoVoice
{
public:
    void setSampleRate(double sr)
    {
        sampleRate = sr;
        osc.setSampleRate(sr);
        osc2.setSampleRate(sr);
        filter.setSampleRate(sr);
        ampEnv.setSampleRate(sr);
        filterEnv.setSampleRate(sr);
    }

    // Detune offset in cents (used for unison spread)
    void setDetuneOffset(float cents) { detuneCents = cents; }

    // Global fine tune in cents (master tuning)
    void setGlobalFineTune(float cents) { globalFineCents = cents; }

    // Pitch bend in semitones (typically ±2)
    void setPitchBend(float semitones) { pitchBendSemitones = semitones; }

    // Portamento: 0 = instant, 1 = max glide (~2 seconds)
    void setPortamento(float amount) { portamentoAmount = amount; }

    // Voice age tracking (P2-5: oldest-released voice stealing)
    void setAgeStamp(uint64_t stamp) { ageStamp = stamp; }
    uint64_t getAgeStamp() const { return ageStamp; }
    bool isInRelease() const { return ampEnv.isInRelease(); }

    void noteOn(int midiNote, float velocity)
    {
        currentNote = midiNote;
        currentVelocity = velocity;
        float freq = 440.0f * std::pow(2.0f, (midiNote - 69) / 12.0f);
        // Apply global fine tune + unison detune offset + pitch bend
        float totalDetune = detuneCents + globalFineCents;
        if (std::abs(totalDetune) > 0.01f)
            freq *= std::pow(2.0f, totalDetune / 1200.0f);
        if (std::abs(pitchBendSemitones) > 0.001f)
            freq *= std::pow(2.0f, pitchBendSemitones / 12.0f);

        targetFrequency = freq;

        // OSC2 frequency with octave offset and fine detune
        float osc2Freq = freq * std::pow(2.0f, (float)osc2Octave) * std::pow(2.0f, osc2FineCents / 1200.0f);
        osc2TargetFreq = osc2Freq;

        // Portamento: if already playing, glide from current freq
        if (active && portamentoAmount > 0.001f)
        {
            float denom = portamentoAmount * (float)sampleRate * 0.5f;
            if (denom > 0.001f)
                portamentoCoeff = 1.0f - std::pow(0.999f, 1.0f / denom);
            else
                portamentoCoeff = 1.0f; // instant if denominator too small
        }
        else
        {
            currentFrequency = freq;
            osc.setFrequency(freq);
            osc.reset();
            osc2CurrentFreq = osc2Freq;
            osc2.setFrequency(osc2Freq);
            osc2.reset();
            filter.reset();
            portamentoCoeff = 1.0f; // instant
        }
        ampEnv.noteOn();
        filterEnv.noteOn();
        active = true;
    }

    void noteOff()
    {
        ampEnv.noteOff();
        filterEnv.noteOff();
    }

    bool isActive() const { return active; }
    int getCurrentNote() const { return currentNote; }

    // --- OSC1 parameters ---
    void setSawLevel(float v)      { osc.setSawLevel(v); }
    void setPulseLevel(float v)    { osc.setPulseLevel(v); }
    void setTriangleLevel(float v) { osc.setTriangleLevel(v); }
    void setSubLevel(float v)      { osc.setSubLevel(v); }
    void setNoiseLevel(float v)    { osc.setNoiseLevel(v); }
    void setPulseWidth(float v)    { osc.setPulseWidth(v); }

    // --- OSC2 parameters ---
    void setOsc2SawLevel(float v)      { osc2.setSawLevel(v); }
    void setOsc2PulseLevel(float v)    { osc2.setPulseLevel(v); }
    void setOsc2TriangleLevel(float v) { osc2.setTriangleLevel(v); }
    void setOsc2PulseWidth(float v)    { osc2BasePW = v; }  // set base PW for LFO modulation
    void setOsc2Octave(int oct)     { osc2Octave = std::clamp(oct, -2, 2); }
    void setOsc2Fine(float cents)   { osc2FineCents = std::clamp(cents, -100.0f, 100.0f); }

    // --- Filter ---
    void setFilterCutoff(float v)      { baseCutoff = v; }
    void setFilterResonance(float v)   { filter.setResonance(v); }
    void setFilterEnvAmount(float v)   { filterEnvAmount = v; }
    void setBasePulseWidth(float v)    { basePW = v; }

    /**
     * Apply per-sample LFO modulation offsets.
     * Called before process() each sample.
     */
    void applyLFOMod(float cutoffMod, float pwMod, float pitchSemitones)
    {
        lfoCutoffOffset = cutoffMod;
        lfoPwOffset = pwMod;

        // Pitch modulation: shift frequency by semitones (+ unison detune + global fine tune + pitch bend)
        float totalDetune = detuneCents + globalFineCents;
        float totalPitchMod = pitchSemitones + pitchBendSemitones;
        if ((std::abs(totalPitchMod) > 0.001f || std::abs(totalDetune) > 0.01f) && currentNote >= 0)
        {
            float baseFreq = 440.0f * std::pow(2.0f, (currentNote - 69) / 12.0f);
            if (std::abs(totalDetune) > 0.01f)
                baseFreq *= std::pow(2.0f, totalDetune / 1200.0f);
            float modFreq = baseFreq * std::pow(2.0f, totalPitchMod / 12.0f);
            osc.setFrequency(modFreq);

            // OSC2 also gets pitch mod
            float osc2Base = baseFreq * std::pow(2.0f, (float)osc2Octave) * std::pow(2.0f, osc2FineCents / 1200.0f);
            float osc2Mod = osc2Base * std::pow(2.0f, totalPitchMod / 12.0f);
            osc2.setFrequency(osc2Mod);
        }
    }

    void setAmpADSR(float a, float d, float s, float r)
    {
        ampEnv.setAttack(a);
        ampEnv.setDecay(d);
        ampEnv.setSustain(s);
        ampEnv.setRelease(r);
    }

    void setFilterADSR(float a, float d, float s, float r)
    {
        filterEnv.setAttack(a);
        filterEnv.setDecay(d);
        filterEnv.setSustain(s);
        filterEnv.setRelease(r);
    }

    float process()
    {
        if (!active) return 0.0f;

        // Get envelope values
        float ampLevel = ampEnv.process();
        float filterMod = filterEnv.process();

        if (!ampEnv.isActive())
        {
            active = false;
            return 0.0f;
        }

        // Apply portamento (glide between frequencies) for both oscs
        if (std::abs(currentFrequency - targetFrequency) > 0.01f)
        {
            currentFrequency += (targetFrequency - currentFrequency) * portamentoCoeff;
            osc.setFrequency(currentFrequency);
        }
        if (std::abs(osc2CurrentFreq - osc2TargetFreq) > 0.01f)
        {
            osc2CurrentFreq += (osc2TargetFreq - osc2CurrentFreq) * portamentoCoeff;
            osc2.setFrequency(osc2CurrentFreq);
        }

        // Apply LFO pulse width modulation (both oscs)
        float modPW = std::clamp(basePW + lfoPwOffset, 0.05f, 0.95f);
        osc.setPulseWidth(modPW);
        // OSC2 PW is set independently but also gets LFO mod
        float modPW2 = std::clamp(osc2BasePW + lfoPwOffset, 0.05f, 0.95f);
        osc2.setPulseWidth(modPW2);

        // Generate both oscillators and mix
        float sample = osc.process() + osc2.process();

        // Apply filter with envelope modulation + LFO cutoff mod
        float cutoffMod = baseCutoff + filterEnvAmount * filterMod * 10000.0f + lfoCutoffOffset;
        cutoffMod = std::clamp(cutoffMod, 20.0f, 20000.0f);
        filter.setCutoff(cutoffMod);
        sample = filter.process(sample);

        // Apply amp envelope and velocity
        sample *= ampLevel * currentVelocity;

        return sample;
    }

private:
    double sampleRate = 44100.0;
    bool active = false;
    int currentNote = -1;
    float currentVelocity = 0.0f;

    JunoOscillator osc;    // OSC1
    JunoOscillator osc2;   // OSC2
    JunoFilter filter;
    JunoEnvelope ampEnv;
    JunoEnvelope filterEnv;

    float baseCutoff = 10000.0f;
    float basePW = 0.5f;
    float filterEnvAmount = 0.5f;

    // OSC2 settings
    int osc2Octave = 0;           // -2 to +2
    float osc2FineCents = 0.0f;   // -100 to +100 cents
    float osc2BasePW = 0.5f;
    float osc2CurrentFreq = 440.0f;
    float osc2TargetFreq = 440.0f;

    // LFO modulation offsets (set per sample)
    float lfoCutoffOffset = 0.0f;
    float lfoPwOffset = 0.0f;

    // Unison detune (cents)
    float detuneCents = 0.0f;

    // Global fine tune (cents)
    float globalFineCents = 0.0f;

    // Pitch bend (semitones)
    float pitchBendSemitones = 0.0f;

    // Portamento
    float portamentoAmount = 0.0f;
    float portamentoCoeff = 1.0f;
    float currentFrequency = 440.0f;
    float targetFrequency = 440.0f;

    // Voice-stealing age stamp (set by processor at each noteOn)
    uint64_t ageStamp = 0;
};
