#pragma once
#include <array>
#include <functional>
#include <cmath>
#include <random>
#include <atomic>
#include <juce_core/juce_core.h>

/**
 * StepSequencer — 16-step sequencer synced to DAW tempo
 * Each step has: note (MIDI), velocity, gate on/off, glide, accent.
 * Supports variable step count (1-16), swing, and gate length.
 *
 * Thread safety: Step fields are std::atomic so UI thread mutations
 * (toggleStep, setStepNote, randomize, loadPattern…) don't race
 * with audio thread reads in process()/triggerCurrentStep().
 */
class StepSequencer
{
public:
    static constexpr int MAX_STEPS = 16;

    struct Step
    {
        std::atomic<int>   note{60};        // MIDI note (C4 default)
        std::atomic<float> velocity{0.8f};  // 0-1
        std::atomic<bool>  active{false};   // gate on/off
        std::atomic<bool>  glide{false};    // legato to next step
        std::atomic<bool>  accent{false};   // accent (higher velocity)

        // Atomic fields can't be copied/assigned implicitly; provide explicit copy
        // so std::array<Step,...> default-construction still compiles.
        Step() = default;
        Step(const Step& o) noexcept
            : note(o.note.load(std::memory_order_relaxed)),
              velocity(o.velocity.load(std::memory_order_relaxed)),
              active(o.active.load(std::memory_order_relaxed)),
              glide(o.glide.load(std::memory_order_relaxed)),
              accent(o.accent.load(std::memory_order_relaxed)) {}
        Step& operator=(const Step& o) noexcept
        {
            note.store(o.note.load(std::memory_order_relaxed), std::memory_order_relaxed);
            velocity.store(o.velocity.load(std::memory_order_relaxed), std::memory_order_relaxed);
            active.store(o.active.load(std::memory_order_relaxed), std::memory_order_relaxed);
            glide.store(o.glide.load(std::memory_order_relaxed), std::memory_order_relaxed);
            accent.store(o.accent.load(std::memory_order_relaxed), std::memory_order_relaxed);
            return *this;
        }
    };

    // Snapshot for callers that want to inspect a step without dealing with atomics.
    struct StepSnapshot
    {
        int note = 60;
        float velocity = 0.8f;
        bool active = false;
        bool glide = false;
        bool accent = false;
    };

    // Callbacks
    std::function<void(int note, float velocity)> onNoteOn;
    std::function<void(int note)> onNoteOff;
    /** Glide step: hand the sounding voice over to a new pitch instead of
        starting a second one. See JunoVoice::glideTo. */
    std::function<void(int fromNote, int toNote, float velocity)> onNoteGlide;

    void setSampleRate(double sr) { sampleRate = sr; }

    // Transport
    void setPlaying(bool p)
    {
        if (p && !playing)
        {
            // Starting: reset position
            sampleCounter = 0;
            currentStep.store(0, std::memory_order_release);
            lastNoteOn = -1;
        }
        if (!p && playing)
        {
            // Stopping: kill any playing note
            if (lastNoteOn >= 0 && onNoteOff)
                onNoteOff(lastNoteOn);
            lastNoteOn = -1;
        }
        playing = p;
    }

    bool isPlaying() const { return playing; }
    int getCurrentStep() const { return currentStep.load(std::memory_order_acquire); }
    int getLastPlayingNote() const { return lastNoteOn; }

    // Rate divisions — same as arpeggiator
    enum RateDiv {
        Rate_1_1 = 0, Rate_1_2, Rate_1_4, Rate_1_8, Rate_1_16, Rate_1_32,
        Rate_1_4T, Rate_1_8T, Rate_1_16T, NUM_RATES
    };

    static constexpr double rateDivBeats[NUM_RATES] = {
        4.0,   // 1/1
        2.0,   // 1/2
        1.0,   // 1/4
        0.5,   // 1/8
        0.25,  // 1/16 (default)
        0.125, // 1/32
        0.6667,// 1/4T
        0.3333,// 1/8T
        0.1667 // 1/16T
    };

    void setRateDiv(int idx) { rateDiv = std::clamp(idx, 0, (int)NUM_RATES - 1); }
    int getRateDiv() const { return rateDiv; }

    // Parameters
    void setTempo(double bpm) { tempo = std::max(bpm, 1.0); }
    void setNumSteps(int n) { numSteps.store(std::clamp(n, 1, MAX_STEPS), std::memory_order_release); }
    void setSwing(float s) { swing = std::clamp(s, 0.0f, 0.7f); }
    void setGateLength(float g) { gateLength = std::clamp(g, 0.05f, 1.0f); }

    int getNumSteps() const { return numSteps.load(std::memory_order_acquire); }
    float getSwing() const { return swing; }
    float getGateLength() const { return gateLength; }

    // Step access — returns a snapshot (atomic loads in one place).
    // Returned by value, with plain (non-atomic) fields, so existing call
    // sites that do `step.active`, `step.note` etc keep working unchanged.
    StepSnapshot getStep(int idx) const
    {
        const auto& s = steps[idx % MAX_STEPS];
        StepSnapshot snap;
        snap.note     = s.note.load(std::memory_order_acquire);
        snap.velocity = s.velocity.load(std::memory_order_acquire);
        snap.active   = s.active.load(std::memory_order_acquire);
        snap.glide    = s.glide.load(std::memory_order_acquire);
        snap.accent   = s.accent.load(std::memory_order_acquire);
        return snap;
    }

    // Set step note
    void setStepNote(int idx, int note)    { steps[idx % MAX_STEPS].note.store(std::clamp(note, 0, 127), std::memory_order_release); }
    void setStepActive(int idx, bool a)    { steps[idx % MAX_STEPS].active.store(a, std::memory_order_release); }
    void setStepGlide(int idx, bool g)     { steps[idx % MAX_STEPS].glide.store(g, std::memory_order_release); }
    void setStepVelocity(int idx, float v) { steps[idx % MAX_STEPS].velocity.store(std::clamp(v, 0.0f, 1.0f), std::memory_order_release); }
    void setStepAccent(int idx, bool a)    { steps[idx % MAX_STEPS].accent.store(a, std::memory_order_release); }

    // Convenience wrappers for UI
    bool isStepActive(int idx) const { return steps[idx % MAX_STEPS].active.load(std::memory_order_acquire); }
    bool hasGlide(int idx) const     { return steps[idx % MAX_STEPS].glide.load(std::memory_order_acquire); }
    bool hasAccent(int idx) const    { return steps[idx % MAX_STEPS].accent.load(std::memory_order_acquire); }

    void toggleStep(int idx)   { auto& s = steps[idx % MAX_STEPS]; s.active.store(!s.active.load(std::memory_order_acquire), std::memory_order_release); }
    void toggleGlide(int idx)  { auto& s = steps[idx % MAX_STEPS]; s.glide.store(!s.glide.load(std::memory_order_acquire), std::memory_order_release); }
    void toggleAccent(int idx) { auto& s = steps[idx % MAX_STEPS]; s.accent.store(!s.accent.load(std::memory_order_acquire), std::memory_order_release); }

    void transposeStep(int idx, int semitones)
    {
        auto& s = steps[idx % MAX_STEPS];
        int cur = s.note.load(std::memory_order_acquire);
        s.note.store(std::clamp(cur + semitones, 0, 127), std::memory_order_release);
    }

    void randomizePattern() { randomize(); }
    void clearPattern() { clearAllSteps(); }

    // Clear all steps
    void clearAllSteps()
    {
        for (auto& s : steps)
        {
            s.active.store(false, std::memory_order_release);
            s.note.store(60, std::memory_order_release);
            s.velocity.store(0.8f, std::memory_order_release);
            s.glide.store(false, std::memory_order_release);
            s.accent.store(false, std::memory_order_release);
        }
    }

    // Process: call once per audio sample
    void process()
    {
        if (!playing || tempo <= 0.0) return;

        // Samples per step based on rate division
        double samplesPerBeat = sampleRate * 60.0 / tempo;
        double samplesPerStep = samplesPerBeat * rateDivBeats[rateDiv];

        // Load current step and num steps atomically
        int cur = currentStep.load(std::memory_order_acquire);
        int numStepsVal = numSteps.load(std::memory_order_acquire);

        // Apply swing to even steps (0-indexed: steps 1,3,5... get delayed)
        double stepLength = samplesPerStep;
        if (cur % 2 == 1)
            stepLength = samplesPerStep * (1.0 + swing);
        else if (cur % 2 == 0 && cur > 0)
            stepLength = samplesPerStep * (1.0 - swing * 0.5);

        // Gate off timing
        double gateOffSample = stepLength * gateLength;

        // Check if we need to send gate off
        if (gateActive && sampleCounter >= (long long)gateOffSample)
        {
            // Don't send note off if next step has glide AND is active
            int nextIdx = (cur + 1) % numStepsVal;
            bool nextGlide = steps[nextIdx].active.load(std::memory_order_acquire)
                          && steps[nextIdx].glide.load(std::memory_order_acquire);

            if (!nextGlide)
            {
                if (lastNoteOn >= 0 && onNoteOff)
                    onNoteOff(lastNoteOn);
                lastNoteOn = -1;
            }
            // Note: if nextGlide is true, we keep lastNoteOn set so that
            // triggerCurrentStep() can send note-off before the new note-on.
            gateActive = false;
        }

        // Check if step advances
        if (sampleCounter >= (long long)stepLength)
        {
            sampleCounter = 0;
            int nextStep = (cur + 1) % numStepsVal;
            currentStep.store(nextStep, std::memory_order_release);
            triggerCurrentStep();
        }

        sampleCounter++;
    }

    // Sync to DAW position (call from processBlock with playhead info)
    void syncToHost(double ppqPosition, double bpm, bool hostPlaying)
    {
        tempo = bpm;

        if (hostPlaying && !playing)
        {
            playing = true;
            // Calculate which step we're on from PPQ using rate division
            double stepsFromPPQ = ppqPosition / rateDivBeats[rateDiv];
            int numStepsVal = numSteps.load(std::memory_order_acquire);
            // Positive modulo: host pre-roll/count-in gives negative PPQ, and
            // C++ % of a negative yields a negative index → out-of-bounds.
            int rawStep = (int)std::floor(stepsFromPPQ);
            int newStep = ((rawStep % numStepsVal) + numStepsVal) % numStepsVal;
            currentStep.store(newStep, std::memory_order_release);
            sampleCounter = 0;
            triggerCurrentStep();
        }
        else if (!hostPlaying && playing)
        {
            setPlaying(false);
        }

        // Continuous PPQ resync to prevent drift
        if (hostPlaying && playing)
            resyncFromPPQ(ppqPosition, bpm);
    }

    // Continuous PPQ resync — correct accumulated drift each block
    // Includes double-trigger guard to prevent retriggering same step within tolerance
    void resyncFromPPQ(double ppqPosition, double bpm)
    {
        if (!playing) return;
        tempo = bpm;

        double samplesPerBeat = sampleRate * 60.0 / bpm;
        double beatsPerStep = rateDivBeats[rateDiv];
        double samplesPerStep = samplesPerBeat * beatsPerStep;

        double stepsFromPPQ = ppqPosition / beatsPerStep;
        int numStepsVal = numSteps.load(std::memory_order_acquire);
        // Positive modulo guards against negative PPQ (host pre-roll/count-in).
        int rawStep = (int)std::floor(stepsFromPPQ);
        int expectedStep = ((rawStep % numStepsVal) + numStepsVal) % numStepsVal;

        // How far into the current step (in samples)
        double fractional = stepsFromPPQ - std::floor(stepsFromPPQ);
        long long expectedCounter = (long long)(fractional * samplesPerStep);

        int cur = currentStep.load(std::memory_order_acquire);
        if (cur != expectedStep)
        {
            // Double-trigger guard: only retrigger if we're early enough in the new step
            // If we're past 10% of the step, it was already triggered by process()
            if (fractional < 0.1)
            {
                currentStep.store(expectedStep, std::memory_order_release);
                sampleCounter = expectedCounter;
                triggerCurrentStep();
            }
            else
            {
                // Late in the step — just correct position without retriggering
                currentStep.store(expectedStep, std::memory_order_release);
                sampleCounter = expectedCounter;
            }
        }
        else
        {
            // Same step — just correct sub-step drift
            sampleCounter = expectedCounter;
        }
    }

    // ------------------------------------------------------------------
    //  Randomize — a musical line, not noise inside a scale.
    //
    //  The previous version drew an octave (0 or +12) AND a scale degree
    //  independently for EVERY step, and switched each step on with a coin
    //  flip. That gives no melodic contour at all: the line leaps an octave
    //  at random, and with 20% of steps gliding, the portamento smeared
    //  across those leaps — which is what made it sound out of tune rather
    //  than musical.
    //
    //  Now: a rhythmic template decides WHICH steps play, the pitch follows
    //  a random walk that prefers stepwise motion, the octave changes only
    //  deliberately on a downbeat, and glide is reserved for small intervals.
    // ------------------------------------------------------------------
    void randomize()
    {
        static const int minorPenta[] = { 0, 3, 5, 7, 10 };            // 5 notes
        static const int minorScale[] = { 0, 2, 3, 5, 7, 8, 10 };      // natural minor
        static const int roots[] = { 36, 38, 40, 41, 43, 45 };         // C2..A2

        // Rhythms that actually groove, instead of a per-step coin flip.
        static const unsigned short rhythms[] = {
            0xAAAA,  // 1010101010101010 — straight eighths
            0x8A8A,  // 1000101010001010 — sparse, syncopated
            0xBABA,  // 1011101010111010 — driving
            0xA2A2,  // 1010001010100010 — laid back
            0xFAFA   // 1111101011111010 — busy 303 roll
        };

        juce::SpinLock::ScopedLockType lock(rngLock);
        auto randInt = [this](int n) { return std::uniform_int_distribution<int>(0, n - 1)(rng); };

        const int  root     = roots[randInt(6)];
        const bool usePenta = randInt(10) < 7;
        const int* scale    = usePenta ? minorPenta : minorScale;
        const int  scaleLen = usePenta ? 5 : 7;

        const unsigned short mask = rhythms[randInt(5)];

        int degree   = 0;       // current scale degree
        int octave   = 0;       // 0 or 1 — moves rarely, on purpose
        int prevNote = root;

        for (int i = 0; i < MAX_STEPS; ++i)
        {
            const bool on = ((mask >> (15 - i)) & 1) != 0;
            steps[i].active.store(on, std::memory_order_release);

            if (!on)
            {
                steps[i].glide.store(false, std::memory_order_release);
                steps[i].accent.store(false, std::memory_order_release);
                continue;
            }

            // Stepwise-biased random walk: mostly repeat or move one degree.
            const int r = randInt(10);
            const int delta = (r < 3) ? 0 : (r < 6) ? 1 : (r < 8) ? -1 : (r < 9) ? 2 : -2;
            degree += delta;
            while (degree >= scaleLen) { degree -= scaleLen; octave = 1; }
            while (degree <  0)        { degree += scaleLen; octave = 0; }

            // An octave drop is a musical decision, so only on a downbeat.
            if ((i % 8) == 0 && randInt(4) == 0)
                octave = 0;

            const int note = root + octave * 12 + scale[degree];
            steps[i].note.store(note, std::memory_order_release);
            steps[i].velocity.store(0.55f + randInt(35) / 100.0f, std::memory_order_release);

            // Glide only makes sense between neighbouring pitches, and sparingly.
            const int interval = std::abs(note - prevNote);
            steps[i].glide.store(interval > 0 && interval <= 4 && randInt(7) == 0,
                                 std::memory_order_release);

            // Accents land on the beat far more often than off it.
            steps[i].accent.store(((i % 4) == 0) ? (randInt(2) == 0) : (randInt(8) == 0),
                                  std::memory_order_release);

            prevNote = note;
        }

        // Anchor the downbeat on the root so the pattern has a home.
        steps[0].active.store(true, std::memory_order_release);
        steps[0].note.store(root, std::memory_order_release);
        steps[0].accent.store(true, std::memory_order_release);
        steps[0].glide.store(false, std::memory_order_release);
    }

    // ============================================================
    //  Pattern Presets
    // ============================================================
    struct PatternPreset
    {
        const char* name;
        const char* genre; // "belgian" or "italian"
        struct StepData { int note; bool active; bool glide; bool accent; };
        StepData data[MAX_STEPS];
    };

    static constexpr int NUM_PATTERNS = 24;

    static const PatternPreset& getPattern(int index)
    {
        // ============================================================
        //  TSYGANATOR SIGNATURE PATTERNS
        //  Not generic sequences — these are the Tsyganator's voice.
        //  Belgian: C minor pentatonic — raw, acid, EBM warehouse energy
        //  Italian: A minor / D minor — melodic, dramatic, Italo romance
        //  C2=36 D2=38 Eb2=39 E2=40 F2=41 G2=43 Ab2=44
        //  A2=45 Bb2=46 B2=47 C3=48 D3=50 Eb3=51 E3=52 F3=53 G3=55
        // ============================================================

        static const PatternPreset patterns[NUM_PATTERNS] = {
            // ============================================================
            //  P31 — 24 patterns, each a Tsyganator-style nod to a
            //  genre-defining classic. Bassline shape & key follow the
            //  source; the name lifts the spirit without copying the
            //  trademark. Range C2..C4 (MIDI 36..60).
            //
            //  BELGIAN (0-11) — New Beat / EBM
            // ============================================================

            // 0: "Antwerpen Acid" — Phuture / "Acid Tracks" (1987). Cm 303.
            { "Antwerpen Acid", "belgian", {
                {36,true,false,true}    , {36,false,false,false}  , {48,true,false,false}   , {46,true,true,false},
                {43,true,true,true}     , {43,false,false,false}  , {48,true,false,true}    , {48,true,false,false},
                {39,true,false,true}    , {36,true,true,false}    , {39,false,false,false}  , {43,true,false,false},
                {39,true,true,true}     , {36,true,false,false}   , {39,true,true,false}    , {36,true,false,false}
            }},

            // 1: "Tsygan 242" — Front 242 / "Headhunter" (1988). Cm industrial stomp.
            { "Tsygan 242", "belgian", {
                {36,true,false,true}    , {36,true,false,false}   , {36,false,false,false}  , {36,true,false,false},
                {43,true,true,true}     , {36,true,false,false}   , {36,false,false,false}  , {43,true,false,false},
                {36,true,false,true}    , {39,true,false,false}   , {36,false,false,false}  , {43,true,false,false},
                {36,true,false,true}    , {41,true,false,false}   , {43,false,false,false}  , {36,true,false,true}
            }},

            // 2: "Chant Nitzer" — Nitzer Ebb / "Join in the Chant" (1986). F#m drone.
            { "Chant Nitzer", "belgian", {
                {42,true,false,true}    , {42,false,false,false}  , {42,false,false,false}  , {49,true,true,true},
                {42,true,false,false}   , {42,true,false,false}   , {49,false,false,false}  , {42,true,false,true},
                {42,true,false,false}   , {42,true,false,false}   , {42,false,false,false}  , {49,true,false,true},
                {42,true,false,false}   , {42,true,false,false}   , {45,true,false,true}    , {49,true,false,false}
            }},

            // 3: "Flesh Wallon" — A Split Second / "Flesh" (1986). Dm New Beat sync.
            { "Flesh Wallon", "belgian", {
                {38,true,false,true}    , {38,false,false,false}  , {38,true,false,false}   , {38,false,false,false},
                {41,true,true,true}     , {41,false,false,false}  , {38,true,false,false}   , {45,true,false,true},
                {38,true,false,false}   , {38,false,false,false}  , {38,true,false,false}   , {38,false,false,false},
                {48,true,false,true}    , {45,true,false,false}   , {41,true,false,false}   , {38,true,false,true}
            }},

            // 4: "Confettis Sauvages" — Confetti's / "The Sound of C" (1988). C hardstop.
            { "Confettis Sauvages", "belgian", {
                {36,true,false,true}    , {36,false,false,false}  , {36,true,false,false}   , {36,false,false,false},
                {43,true,true,true}     , {43,false,false,false}  , {43,true,false,false}   , {43,false,false,false},
                {36,true,false,true}    , {40,true,false,false}   , {36,false,false,false}  , {43,true,false,true},
                {48,true,false,false}   , {43,true,false,true}    , {40,false,false,false}  , {36,true,false,false}
            }},

            // 5: "Sit on Speculoos" — Lords of Acid / "I Sit On Acid" (1988). Am acid+slides.
            { "Sit on Speculoos", "belgian", {
                {45,true,false,true}    , {45,true,false,false}   , {57,true,false,false}   , {43,false,false,false},
                {40,true,false,true}    , {45,true,true,false}    , {48,true,true,false}    , {40,false,false,false},
                {45,true,false,false}   , {45,true,false,true}    , {52,true,true,false}    , {48,false,false,false},
                {45,true,false,true}    , {43,true,true,false}    , {40,true,false,false}   , {45,false,false,false}
            }},

            // 6: "Der Bruxellois" — DAF / "Der Mussolini" (1981). Em proto-EBM 8ths.
            { "Der Bruxellois", "belgian", {
                {40,true,false,true}    , {40,false,false,false}  , {40,true,false,false}   , {40,true,false,false},
                {40,false,false,false}  , {40,true,false,false}   , {47,false,false,false}  , {40,true,false,false},
                {40,true,false,false}   , {40,false,false,false}  , {40,true,false,false}   , {47,true,true,true},
                {40,false,false,false}  , {43,true,false,true}    , {47,true,false,false}   , {40,false,false,false}
            }},

            // 7: "Ninos Bruxelles" — Liaisons Dangereuses / "Los Niños del Parque" (1981). Bm proto-EBM.
            { "Ninos Bruxelles", "belgian", {
                {47,true,false,true}    , {47,true,false,false}   , {50,false,false,false}  , {47,true,false,false},
                {42,true,true,true}     , {47,true,false,false}   , {50,false,false,false}  , {54,true,false,true},
                {47,true,true,false}    , {47,true,false,false}   , {50,false,false,false}  , {47,true,false,false},
                {45,true,false,false}   , {50,true,false,false}   , {54,false,false,false}  , {47,true,false,false}
            }},

            // 8: "Quelle Heure Love" — KLF / "What Time Is Love" (1988). Dm acid bounce.
            { "Quelle Heure Love", "belgian", {
                {38,false,false,false}  , {38,false,false,false}  , {50,true,false,false}   , {38,true,false,false},
                {45,true,true,true}     , {38,false,false,false}  , {50,true,false,false}   , {45,true,false,false},
                {38,true,false,true}    , {41,true,true,false}    , {45,true,false,true}    , {50,true,false,false},
                {45,true,false,true}    , {41,false,false,false}  , {50,true,false,false}   , {38,true,false,false}
            }},

            // 9: "Mons Motion" — Cubic 22 / "Night in Motion" (1991). Gm ascending.
            { "Mons Motion", "belgian", {
                {43,true,false,true}    , {43,false,false,false}  , {46,true,true,false}    , {50,false,false,false},
                {43,true,false,false}   , {46,true,false,false}   , {48,true,true,false}    , {50,false,false,false},
                {43,true,false,false}   , {46,true,false,false}   , {50,true,false,false}   , {53,true,true,true},
                {43,true,false,false}   , {50,true,false,false}   , {46,true,false,false}   , {43,true,false,false}
            }},

            // 10: "Tsygan Assimilate" — Skinny Puppy / "Assimilate" (1985). Am EBM crawl.
            { "Tsygan Assimilate", "belgian", {
                {45,true,false,true}    , {45,false,false,false}  , {45,true,false,false}   , {45,false,false,false},
                {48,true,true,true}     , {48,false,false,false}  , {45,true,false,false}   , {43,true,false,true},
                {40,true,false,false}   , {40,false,false,false}  , {40,true,false,false}   , {43,true,false,true},
                {45,true,false,false}   , {43,false,false,false}  , {40,true,false,false}   , {45,true,false,false}
            }},

            // 11: "Move Le Boudin" — Erotic Dissidents / "Move Your Ass…" (1988). Cm groove.
            { "Move Le Boudin", "belgian", {
                {36,true,false,true}    , {36,false,false,false}  , {36,false,false,false}  , {39,true,true,false},
                {36,false,false,false}  , {43,true,false,true}    , {36,true,false,false}   , {46,true,false,false},
                {36,false,false,false}  , {39,true,false,true}    , {43,true,false,false}   , {46,true,true,false},
                {48,true,false,false}   , {46,true,false,true}    , {43,true,false,false}   , {36,true,false,false}
            }},

            // ============================================================
            //  ITALIAN (12-23) — Italo Disco
            // ============================================================

            // 12: "Saturnia Heat" — Moroder / Donna Summer "I Feel Love" (1977). C octave-bounce.
            { "Saturnia Heat", "italian", {
                {36,true,false,true}    , {48,true,false,false}   , {36,true,false,false}   , {48,true,false,false},
                {36,true,false,true}    , {48,true,false,false}   , {36,true,false,false}   , {48,false,false,false},
                {36,true,false,false}   , {48,true,false,true}    , {36,true,false,false}   , {48,true,false,false},
                {36,true,false,true}    , {48,true,false,false}   , {36,true,false,false}   , {48,false,false,false}
            }},

            // 13: "Roma Sauvage" — Baltimora / "Tarzan Boy" (1985). Em syncopated hook.
            { "Roma Sauvage", "italian", {
                {40,true,false,true}    , {40,false,false,false}  , {40,true,false,false}   , {43,true,true,false},
                {47,true,false,true}    , {47,false,false,false}  , {47,true,false,false}   , {43,true,false,false},
                {40,true,false,true}    , {40,false,false,false}  , {50,true,false,false}   , {47,true,true,true},
                {45,true,false,false}   , {43,true,false,true}    , {40,false,false,false}  , {38,true,false,false}
            }},

            // 14: "Dolce Tsygan" — Ryan Paris / "Dolce Vita" (1983). Am walking bass.
            { "Dolce Tsygan", "italian", {
                {45,true,false,true}    , {45,true,false,false}   , {52,true,true,false}    , {48,false,false,false},
                {43,true,false,true}    , {43,true,false,false}   , {50,true,false,false}   , {47,false,false,false},
                {41,true,false,false}   , {41,true,false,true}    , {48,true,false,false}   , {45,false,false,false},
                {40,true,false,true}    , {43,true,false,false}   , {47,true,false,false}   , {45,false,false,false}
            }},

            // 15: "Auto-Controllo" — Raf / "Self Control" (1984). Am dramatic octave.
            { "Auto-Controllo", "italian", {
                {45,true,false,true}    , {57,true,false,false}   , {45,true,false,false}   , {57,false,false,false},
                {41,true,false,true}    , {53,true,false,false}   , {41,true,false,false}   , {53,false,false,false},
                {43,true,false,true}    , {55,true,false,false}   , {43,true,false,false}   , {55,false,false,false},
                {40,true,false,true}    , {52,true,false,false}   , {45,true,false,true}    , {57,false,false,false}
            }},

            // 16: "Ragazzi Estate" — Sabrina / "Boys (Summertime Love)" (1987). Am-F-C-G.
            { "Ragazzi Estate", "italian", {
                {45,true,false,true}    , {45,false,false,false}  , {52,false,false,false}  , {48,true,true,false},
                {41,true,false,true}    , {41,false,false,false}  , {48,false,false,false}  , {45,true,false,true},
                {48,true,false,false}   , {48,true,false,true}    , {55,true,true,false}    , {52,true,false,false},
                {43,true,false,true}    , {43,true,false,false}   , {50,true,false,false}   , {47,true,false,false}
            }},

            // 17: "Cervello Futuro" — Den Harrow / "Future Brain" (1985). Dm-Bb-C-A sequence.
            { "Cervello Futuro", "italian", {
                {38,true,false,true}    , {38,false,false,false}  , {41,true,true,false}    , {45,false,false,false},
                {46,true,false,true}    , {46,false,false,false}  , {50,true,false,false}   , {53,true,false,false},
                {48,true,true,true}     , {48,true,false,false}   , {52,true,false,true}    , {55,true,false,false},
                {45,true,false,true}    , {45,true,false,false}   , {49,true,false,false}   , {52,false,false,false}
            }},

            // 18: "Pasta al Chopin" — Gazebo / "I Like Chopin" (1983). Cm descending ballad.
            { "Pasta al Chopin", "italian", {
                {48,true,false,true}    , {48,false,false,false}  , {46,true,true,false}    , {46,false,false,false},
                {44,true,false,true}    , {44,false,false,false}  , {43,true,false,false}   , {43,false,false,false},
                {41,true,false,true}    , {41,false,false,false}  , {39,true,true,false}    , {39,false,false,false},
                {38,true,false,true}    , {36,true,false,false}   , {36,false,false,false}  , {43,true,false,false}
            }},

            // 19: "Parlare Sporco" — Klein & MBO / "Dirty Talk" (1982). Em rhythmic.
            { "Parlare Sporco", "italian", {
                {40,true,false,true}    , {40,false,false,false}  , {40,true,false,false}   , {43,true,true,false},
                {38,true,false,true}    , {38,false,false,false}  , {38,true,false,false}   , {41,true,false,true},
                {36,true,false,false}   , {36,false,false,false}  , {36,true,false,false}   , {39,true,false,false},
                {38,true,false,true}    , {38,false,false,false}  , {47,true,false,false}   , {40,true,false,false}
            }},

            // 20: "La Notte" — Valerie Dore / "The Night" (1984). Am dramatic Am-F-G-Em.
            { "La Notte", "italian", {
                {45,true,false,true}    , {45,false,false,false}  , {57,true,false,false}   , {52,true,true,false},
                {41,true,false,true}    , {41,false,false,false}  , {53,true,false,false}   , {48,true,false,false},
                {43,true,false,true}    , {43,false,false,false}  , {55,true,false,true}    , {50,true,false,false},
                {40,true,false,true}    , {40,true,false,false}   , {43,false,false,false}  , {47,true,false,false}
            }},

            // 21: "Chiamami" — Spagna / "Call Me" (1986). F#m Hi-NRG 4-on-floor.
            { "Chiamami", "italian", {
                {42,true,false,true}    , {42,true,false,false}   , {45,true,true,false}    , {49,false,false,false},
                {49,true,false,true}    , {44,true,false,false}   , {47,true,true,false}    , {49,false,false,false},
                {38,true,false,false}   , {38,true,false,true}    , {42,true,false,false}   , {45,false,false,false},
                {40,true,false,true}    , {40,true,false,false}   , {44,true,false,false}   , {47,false,false,false}
            }},

            // 22: "Pulsar Cosmica" — Hipnosis / "Pulstar" (Vangelis cover, 1983). Cm cosmic sequencer.
            { "Pulsar Cosmica", "italian", {
                {36,true,false,true}    , {43,false,false,false}  , {48,true,false,false}   , {51,true,true,false},
                {55,false,false,false}  , {51,true,false,false}   , {48,true,false,false}   , {43,true,false,false},
                {36,false,false,false}  , {43,true,false,false}   , {46,true,true,true}     , {51,true,false,false},
                {55,true,false,true}    , {53,true,true,false}    , {50,true,false,false}   , {46,true,false,false}
            }},

            // 23: "Calma Albert" — Albert One / "Take It Easy" (1986). Bbm Hi-NRG octave driver.
            { "Calma Albert", "italian", {
                {46,true,false,true}    , {58,true,false,false}   , {46,true,false,false}   , {58,false,false,false},
                {43,true,false,true}    , {55,true,false,false}   , {43,true,false,false}   , {55,false,false,false},
                {44,true,false,true}    , {56,true,false,false}   , {44,true,false,false}   , {56,false,false,false},
                {41,true,false,true}    , {53,true,false,false}   , {46,true,false,true}    , {58,false,false,false}
            }}
        };

        return patterns[std::clamp(index, 0, NUM_PATTERNS - 1)];
    }

    // Load a pattern preset into the sequencer
    void loadPattern(int patternIndex)
    {
        const auto& pattern = getPattern(patternIndex);
        for (int i = 0; i < MAX_STEPS; ++i)
        {
            steps[i].note.store(pattern.data[i].note, std::memory_order_release);
            steps[i].active.store(pattern.data[i].active, std::memory_order_release);
            steps[i].glide.store(pattern.data[i].glide, std::memory_order_release);
            steps[i].accent.store(pattern.data[i].accent, std::memory_order_release);
            steps[i].velocity.store(pattern.data[i].accent ? 1.0f : 0.8f, std::memory_order_release);
        }
        numSteps.store(MAX_STEPS, std::memory_order_release);
    }

    // Get number of Belgian patterns (first N are Belgian).
    // P31: bumped 6 → 12 per genre (24 total) — patterns now imitate
    // genre-defining classics. Belgian = indices 0-11, Italian = 12-23.
    static int getNumBelgianPatterns() { return 12; }
    static int getNumItalianPatterns() { return 12; }
    static int getFirstItalianPatternIndex() { return 12; }

private:
    double sampleRate = 44100.0;
    double tempo = 120.0;
    bool playing = false;
    std::atomic<int> currentStep{ 0 };
    std::atomic<int> numSteps{ 16 };
    float swing = 0.0f;
    float gateLength = 0.5f;
    int rateDiv = Rate_1_16;  // default: 16th notes
    long long sampleCounter = 0;
    bool gateActive = false;
    int lastNoteOn = -1;

    std::array<Step, MAX_STEPS> steps{};

    // Thread-safe RNG for randomize() — guarded by spinLock
    std::mt19937 rng{ std::random_device{}() };
    juce::SpinLock rngLock;

    void triggerCurrentStep()
    {
        int cur = currentStep.load(std::memory_order_acquire);
        // Safety net: never index the steps array out of bounds, even if a
        // desync slipped a bad value through (host pre-roll, race, etc.).
        if (cur < 0 || cur >= MAX_STEPS) return;
        // Snapshot the step in one go — atomic fields, loaded once each
        const bool  stepActive   = steps[cur].active.load(std::memory_order_acquire);
        const int   stepNote     = steps[cur].note.load(std::memory_order_acquire);
        const float stepVelocity = steps[cur].velocity.load(std::memory_order_acquire);
        const bool  stepGlide    = steps[cur].glide.load(std::memory_order_acquire);
        const bool  stepAccent   = steps[cur].accent.load(std::memory_order_acquire);

        if (!stepActive)
        {
            // Inactive step — kill any lingering note from glide
            if (lastNoteOn >= 0 && onNoteOff)
                onNoteOff(lastNoteOn);
            lastNoteOn = -1;
            return;
        }

        // If glide, just change pitch (don't retrigger envelope)
        bool isGlide = stepGlide && lastNoteOn >= 0;

        float vel = stepAccent ? std::min(1.0f, stepVelocity * 1.4f) : stepVelocity;

        if (isGlide && onNoteGlide)
        {
            // Hands the sounding voice to the new pitch. Previously this path
            // sent a bare onNoteOn, which grabbed a second voice and left the
            // first one sustaining with no matching note-off.
            onNoteGlide(lastNoteOn, stepNote, vel);
        }
        else
        {
            if (lastNoteOn >= 0 && onNoteOff)
                onNoteOff(lastNoteOn);
            if (onNoteOn)
                onNoteOn(stepNote, vel);
        }

        lastNoteOn = stepNote;
        gateActive = true;
    }
};
