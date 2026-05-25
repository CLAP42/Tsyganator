#pragma once
#include <cmath>
#include <algorithm>
#include <random>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/**
 * JunoLFO — Simple Low Frequency Oscillator
 * Waveforms: Sine, Triangle, Saw, Square, S&H (Sample & Hold)
 * Destinations: Cutoff, Pulse Width, Pitch, Volume
 * Free-running or sync to note-on.
 */
class JunoLFO
{
public:
    enum Waveform { Sine = 0, Triangle, Saw, Square, SampleAndHold, NumWaveforms };
    enum Destination { ToCutoff = 0, ToPulseWidth, ToPitch, ToVolume, NumDestinations };

    void setSampleRate(double sr) { sampleRate = sr; }

    void setRate(float hz)   { rate = hz; }
    void setDepth(float d)   { depth = d; }  // 0..1
    void setWaveform(int w)  { waveform = static_cast<Waveform>(w); }
    void setDestination(int d) { destination = static_cast<Destination>(d); }

    /**
     * Set LFO rate from a musical sync division and host BPM.
     * syncIndex maps to: 0=4/1, 1=2/1, 2=1/1, 3=1/2, 4=1/4, 5=1/8, 6=1/16, 7=1/32,
     *                     8=1/4T, 9=1/8T, 10=1/16T, 11=1/2., 12=1/4., 13=1/8.
     */
    void setSyncRate(int syncIndex, double bpm)
    {
        // Musical division in beats (quarter notes)
        static constexpr double divisions[] = {
            16.0,   // 4/1   = 16 beats
            8.0,    // 2/1   = 8 beats
            4.0,    // 1/1   = 4 beats
            2.0,    // 1/2   = 2 beats
            1.0,    // 1/4   = 1 beat
            0.5,    // 1/8   = 0.5 beats
            0.25,   // 1/16  = 0.25 beats
            0.125,  // 1/32  = 0.125 beats
            0.6667, // 1/4T  = 2/3 beat (triplet quarter)
            0.3333, // 1/8T  = 1/3 beat (triplet eighth)
            0.1667, // 1/16T = 1/6 beat (triplet sixteenth)
            3.0,    // 1/2.  = 3 beats (dotted half)
            1.5,    // 1/4.  = 1.5 beats (dotted quarter)
            0.75    // 1/8.  = 0.75 beats (dotted eighth)
        };
        int idx = std::clamp(syncIndex, 0, 13);
        double beatsPerCycle = divisions[idx];
        double secondsPerBeat = 60.0 / std::max(bpm, 20.0);
        double cycleDuration = beatsPerCycle * secondsPerBeat;
        rate = (float)(1.0 / std::max(cycleDuration, 0.001));
    }

    float getRate() const       { return rate; }
    float getDepth() const      { return depth; }
    int getWaveform() const     { return (int)waveform; }
    int getDestination() const  { return (int)destination; }

    void reset() { phase = 0.0; shValue = 0.0f; }

    /**
     * Process one sample and return the bipolar modulation value (-1..+1)
     * scaled by depth.
     */
    float process()
    {
        float raw = 0.0f;

        switch (waveform)
        {
            case Sine:
                raw = std::sin(2.0 * M_PI * phase);
                break;

            case Triangle:
            {
                double p = std::fmod(phase + 0.25, 1.0); // shift so starts at 0
                raw = (float)(p < 0.5 ? (4.0 * p - 1.0) : (3.0 - 4.0 * p));
                break;
            }

            case Saw:
                raw = (float)(2.0 * phase - 1.0);
                break;

            case Square:
                raw = phase < 0.5 ? 1.0f : -1.0f;
                break;

            case SampleAndHold:
            {
                double nextPhase = phase + rate / sampleRate;
                if (nextPhase >= 1.0)
                {
                    // Phase about to wrap — generate new random value (thread-safe)
                    shValue = shDist(rng);
                }
                raw = shValue;
                break;
            }

            default: break;
        }

        // Advance phase (with denormal flushing)
        phase += rate / sampleRate;
        if (phase >= 1.0) phase -= 1.0;
        if (phase < 1.0e-15) phase = 0.0;

        return raw * depth;
    }

    /**
     * Apply the LFO modulation to voice parameters.
     * Returns modulation values for each destination.
     */
    float getCutoffMod(float lfoVal) const
    {
        // Scale: 0..1 depth maps to 0..5000 Hz cutoff offset
        return (destination == ToCutoff) ? lfoVal * 5000.0f : 0.0f;
    }

    float getPulseWidthMod(float lfoVal) const
    {
        // Scale: 0..1 depth maps to +/- 0.4 PW
        return (destination == ToPulseWidth) ? lfoVal * 0.4f : 0.0f;
    }

    float getPitchMod(float lfoVal) const
    {
        // Scale: 0..1 depth maps to +/- 2 semitones
        return (destination == ToPitch) ? lfoVal * 2.0f : 0.0f;
    }

    float getVolumeMod(float lfoVal) const
    {
        // Scale: convert bipolar to unipolar (tremolo)
        return (destination == ToVolume) ? (lfoVal * 0.5f + 0.5f) : 1.0f;
    }

private:
    double sampleRate = 44100.0;
    double phase = 0.0;
    float rate = 1.0f;     // Hz
    float depth = 0.0f;    // 0..1
    Waveform waveform = Sine;
    Destination destination = ToCutoff;
    float shValue = 0.0f;  // Sample & Hold stored value

    // Thread-safe RNG for Sample & Hold
    std::mt19937 rng{ std::random_device{}() };
    std::uniform_real_distribution<float> shDist{ -1.0f, 1.0f };
};
