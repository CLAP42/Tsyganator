#pragma once
#include <vector>
#include <algorithm>
#include <functional>
#include <cmath>
#include <random>
#include <juce_core/juce_core.h>

/**
 * Arpeggiator — cycles through held notes in various patterns
 * Syncs to host tempo via the same mechanism as StepSequencer.
 * Modes: Up, Down, Up-Down, Random
 */
class Arpeggiator
{
public:
    enum Mode { Up = 0, Down, UpDown, Random, NUM_MODES };

    std::function<void(int note, float velocity)> onNoteOn;
    std::function<void(int note)> onNoteOff;

    // Arp rate divisions — musical note values
    enum RateDiv {
        Rate_1_1 = 0, Rate_1_2, Rate_1_4, Rate_1_8, Rate_1_16, Rate_1_32,
        Rate_1_4T, Rate_1_8T, Rate_1_16T, NUM_RATES
    };

    static constexpr const char* rateDivNames[NUM_RATES] = {
        "1/1", "1/2", "1/4", "1/8", "1/16", "1/32",
        "1/4T", "1/8T", "1/16T"
    };

    // Beats per step for each rate division
    static constexpr double rateDivBeats[NUM_RATES] = {
        4.0,   // 1/1  = whole note = 4 beats
        2.0,   // 1/2  = half note = 2 beats
        1.0,   // 1/4  = quarter note = 1 beat
        0.5,   // 1/8  = eighth note = 0.5 beats
        0.25,  // 1/16 = sixteenth note = 0.25 beats
        0.125, // 1/32 = thirty-second note = 0.125 beats
        0.6667,// 1/4T = quarter triplet = 2/3 beat
        0.3333,// 1/8T = eighth triplet = 1/3 beat
        0.1667 // 1/16T = sixteenth triplet = 1/6 beat
    };

    void setSampleRate(double sr) { sampleRate = sr; }
    void setTempo(double bpm) { tempo = std::max(bpm, 1.0); }
    void setGateLength(float g) { gateLength = std::clamp(g, 0.05f, 1.0f); }
    void setMode(Mode m) { mode = m; }
    void setOctaveRange(int r) { octaveRange = std::clamp(r, 1, 4); }
    void setRateDiv(int idx) { rateDiv = std::clamp(idx, 0, (int)NUM_RATES - 1); }
    int getRateDiv() const { return rateDiv; }

    Mode getMode() const { return mode; }
    int getOctaveRange() const { return octaveRange; }
    bool isPlaying() const { return playing; }

    // Called when user presses a key
    void notePressed(int note, float velocity)
    {
        // CRITICAL-2: Protect heldNotes vector from concurrent access
        // Called from message thread, locks during read/modify
        juce::SpinLock::ScopedLockType lock(heldNotesLock);

        // Add note if not already held
        for (auto& h : heldNotes)
            if (h.note == note) { h.velocity = velocity; return; }

        heldNotes.push_back({note, velocity});
        sortNotes();
    }

    // Called when user releases a key
    void noteReleased(int note)
    {
        // CRITICAL-2: Protect heldNotes vector from concurrent access
        juce::SpinLock::ScopedLockType lock(heldNotesLock);

        heldNotes.erase(
            std::remove_if(heldNotes.begin(), heldNotes.end(),
                           [note](const HeldNote& h) { return h.note == note; }),
            heldNotes.end());

        // If no notes held, stop
        if (heldNotes.empty())
        {
            if (lastNoteOn >= 0 && onNoteOff)
                onNoteOff(lastNoteOn);
            lastNoteOn = -1;
            currentIndex = 0;
            goingUp = true;
            currentOctave = 0;
        }
    }

    void allNotesOff()
    {
        juce::SpinLock::ScopedLockType lock(heldNotesLock);
        heldNotes.clear();
        if (lastNoteOn >= 0 && onNoteOff)
            onNoteOff(lastNoteOn);
        lastNoteOn = -1;
        currentIndex = 0;
        goingUp = true;
        currentOctave = 0;
    }

    bool hasNotes() const {
        juce::SpinLock::ScopedLockType lock(heldNotesLock);
        return !heldNotes.empty();
    }

    // Sync to DAW transport
    void syncToHost(double ppqPosition, double bpm, bool hostPlaying)
    {
        tempo = bpm;
        if (hostPlaying && !playing)
        {
            playing = true;
            sampleCounter = 0;
            currentIndex = 0;
            currentOctave = 0;
            goingUp = true;
            triggerNext();
        }
        else if (!hostPlaying && playing)
        {
            playing = false;
            if (lastNoteOn >= 0 && onNoteOff)
                onNoteOff(lastNoteOn);
            lastNoteOn = -1;
        }
    }

    void setPlaying(bool p)
    {
        if (p && !playing)
        {
            sampleCounter = 0;
            currentIndex = 0;
            currentOctave = 0;
            goingUp = true;
        }
        if (!p && playing)
        {
            if (lastNoteOn >= 0 && onNoteOff)
                onNoteOff(lastNoteOn);
            lastNoteOn = -1;
        }
        playing = p;
    }

    // Process: call once per audio sample
    void process()
    {
        if (!playing || tempo <= 0.0) return;

        // CRITICAL-2 (P0-3 fix): Take the lock ONCE for the whole step transition.
        // The previous version released the lock between the empty()-check and
        // the advance, which left a window where a release-from-UI could empty
        // heldNotes between the two — leading to advanceIndex() accessing an
        // invalid index. A single lock for the whole RT-sample is cheap (spin
        // lock, contended only on UI events) and eliminates the race.
        juce::SpinLock::ScopedLockType lock(heldNotesLock);

        if (heldNotes.empty()) return;

        double samplesPerBeat = sampleRate * 60.0 / tempo;
        double samplesPerStep = samplesPerBeat * rateDivBeats[rateDiv];

        // Gate off
        double gateOffSample = samplesPerStep * gateLength;
        if (gateActive && sampleCounter >= (long long)gateOffSample)
        {
            if (lastNoteOn >= 0 && onNoteOff)
                onNoteOff(lastNoteOn);
            lastNoteOn = -1;
            gateActive = false;
        }

        // Next step
        if (sampleCounter >= (long long)samplesPerStep)
        {
            sampleCounter = 0;
            advanceIndex();
            triggerNext();
        }

        sampleCounter++;
    }

private:
    struct HeldNote { int note; float velocity; };

    double sampleRate = 44100.0;
    double tempo = 120.0;
    bool playing = false;
    float gateLength = 0.5f;
    int rateDiv = Rate_1_8; // default to 1/8 notes

    std::vector<HeldNote> heldNotes;
    mutable juce::SpinLock heldNotesLock;  // CRITICAL-2: Protects heldNotes vector access
    int currentIndex = 0;
    int currentOctave = 0;
    int octaveRange = 1; // 1 = same octave, 2 = +1 oct, etc.
    bool goingUp = true;
    Mode mode = Up;

    long long sampleCounter = 0;
    bool gateActive = false;
    int lastNoteOn = -1;

    // Thread-safe RNG for Random mode
    std::mt19937 rng{ std::random_device{}() };

    void sortNotes()
    {
        std::sort(heldNotes.begin(), heldNotes.end(),
                  [](const HeldNote& a, const HeldNote& b) { return a.note < b.note; });
    }

    void advanceIndex()
    {
        if (heldNotes.empty()) return;
        int n = (int)heldNotes.size();

        switch (mode)
        {
            case Up:
                currentIndex++;
                if (currentIndex >= n)
                {
                    currentIndex = 0;
                    currentOctave++;
                    if (currentOctave >= octaveRange) currentOctave = 0;
                }
                break;

            case Down:
                currentIndex--;
                if (currentIndex < 0)
                {
                    currentIndex = n - 1;
                    currentOctave++;
                    if (currentOctave >= octaveRange) currentOctave = 0;
                }
                break;

            case UpDown:
                if (goingUp)
                {
                    currentIndex++;
                    if (currentIndex >= n)
                    {
                        if (octaveRange > 1 && currentOctave < octaveRange - 1)
                        {
                            currentIndex = 0;
                            currentOctave++;
                        }
                        else
                        {
                            currentIndex = n - 1;
                            goingUp = false;
                        }
                    }
                }
                else
                {
                    currentIndex--;
                    if (currentIndex < 0)
                    {
                        if (currentOctave > 0)
                        {
                            currentIndex = n - 1;
                            currentOctave--;
                        }
                        else
                        {
                            currentIndex = 0;
                            goingUp = true;
                        }
                    }
                }
                break;

            case Random:
                currentIndex = std::uniform_int_distribution<int>(0, n - 1)(rng);
                currentOctave = (octaveRange > 1) ? std::uniform_int_distribution<int>(0, octaveRange - 1)(rng) : 0;
                break;

            default:
                break;
        }

        currentIndex = std::clamp(currentIndex, 0, n - 1);
    }

    void triggerNext()
    {
        if (heldNotes.empty()) return;

        int idx = std::clamp(currentIndex, 0, (int)heldNotes.size() - 1);
        int note = heldNotes[idx].note + (currentOctave * 12);
        note = std::clamp(note, 0, 127);
        float vel = heldNotes[idx].velocity;

        // Kill previous note
        if (lastNoteOn >= 0 && onNoteOff)
            onNoteOff(lastNoteOn);

        if (onNoteOn)
            onNoteOn(note, vel);

        lastNoteOn = note;
        gateActive = true;
    }
};
