#pragma once
#include <cmath>
#include <algorithm>

/**
 * JunoEnvelope — ADSR envelope generator
 * Analog-style exponential curves for natural sound.
 *
 * P36b fix: when the user moves the sustain knob mid-note, the envelope
 * now smoothly re-targets toward the new level instead of snapping (which
 * produced an audible click in the Sustain stage). The Decay stage also
 * gracefully handles a sustain level that rises above the current output
 * (previously the Decay→Sustain transition test was strictly downward-biased).
 */
class JunoEnvelope
{
public:
    enum Stage { Idle, Attack, Decay, Sustain, Release };

    void setSampleRate(double sr)
    {
        sampleRate = std::max(sr, 1.0);
        // Sustain smoothing coefficient: ~5 ms time constant, click-free re-targeting
        // when the sustain knob is moved live.
        sustainSmoothCoeff = 1.0f - std::exp(-1.0f / (0.005f * (float)sampleRate));
    }

    void setAttack(float seconds)  { attackTime  = std::max(0.001f, seconds); }
    void setDecay(float seconds)   { decayTime   = std::max(0.001f, seconds); }
    void setSustain(float level)   { sustainLevel = std::clamp(level, 0.0f, 1.0f); }
    void setRelease(float seconds) { releaseTime  = std::max(0.001f, seconds); }

    void noteOn()
    {
        stage = Attack;
        // Don't reset output for legato-friendly behavior
    }

    void noteOff()
    {
        if (stage != Idle)
            stage = Release;
    }

    void reset()
    {
        stage = Idle;
        output = 0.0f;
    }

    bool isActive() const { return stage != Idle; }
    Stage getStage() const { return stage; }
    bool isInRelease() const { return stage == Release; }

    float process()
    {
        switch (stage)
        {
            case Idle:
                return 0.0f;

            case Attack:
            {
                float rate = 1.0f / (attackTime * (float)sampleRate);
                output += rate;
                if (output >= 1.0f)
                {
                    output = 1.0f;
                    stage = Decay;
                }
                break;
            }

            case Decay:
            {
                float coeff = expCoeff(decayTime);
                // Re-target each sample: if the user raises sustainLevel above
                // current output mid-decay, the filter naturally glides UP toward
                // the new target (was previously biased toward decreasing values).
                output = sustainLevel + (output - sustainLevel) * coeff;
                // Transition to Sustain stage when we are close enough to target.
                if (std::abs(output - sustainLevel) < 0.0001f)
                {
                    output = sustainLevel;
                    stage = Sustain;
                }
                break;
            }

            case Sustain:
            {
                // P36b: instead of `output = sustainLevel;` (which snapped and
                // clicked when the knob moved), glide toward sustainLevel with a
                // ~5ms time constant. Inaudible at steady state, click-free during
                // live knob automation.
                output += (sustainLevel - output) * sustainSmoothCoeff;
                if (std::abs(output - sustainLevel) < 1.0e-6f)
                    output = sustainLevel;  // snap to exact value once near zero error
                break;
            }

            case Release:
            {
                float coeff = expCoeff(releaseTime);
                output *= coeff;
                if (output < 0.0001f)
                {
                    output = 0.0f;
                    stage = Idle;
                }
                break;
            }
        }

        return output;
    }

private:
    double sampleRate = 44100.0;
    Stage stage = Idle;
    float output = 0.0f;

    float attackTime  = 0.01f;
    float decayTime   = 0.2f;
    float sustainLevel = 0.7f;
    float releaseTime  = 0.3f;

    // P36b: per-sample coefficient that glides output toward sustainLevel during
    // the Sustain stage. Initialised in setSampleRate; ~5ms time constant.
    float sustainSmoothCoeff = 0.0454f;  // sensible default for 44.1kHz

    // Attempt to compute exponential decay coefficient for a given time constant
    float expCoeff(float timeSeconds) const
    {
        return std::exp(-1.0f / (timeSeconds * (float)sampleRate));
    }
};
