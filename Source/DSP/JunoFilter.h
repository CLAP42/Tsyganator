#pragma once
#include <cmath>
#include <algorithm>

/**
 * JunoFilter — 4-pole resonant low-pass filter
 * Transistor ladder model inspired by the Roland IR3109 chip used in the Juno.
 * Provides that creamy, warm Juno low-pass character.
 */
class JunoFilter
{
public:
    void setSampleRate(double sr) { sampleRate = std::max(sr, 1.0); }

    void setCutoff(float freq)
    {
        cutoff = std::clamp(freq, 20.0f, 20000.0f);
    }

    void setResonance(float res)
    {
        resonance = std::clamp(res, 0.0f, 1.0f);
    }

    void reset()
    {
        s1 = s2 = s3 = s4 = 0.0f;
    }

    float process(float input)
    {
        // Clamp cutoff to stay well below Nyquist (tan approaches infinity at pi/2)
        // Use 0.45 to be safe even at very low sample rates (22050 Hz)
        float safeCutoff = std::min(cutoff, (float)(sampleRate * 0.45));

        // Frequency warping for accurate cutoff at high frequencies
        float wc = 2.0f * std::tan(3.14159265f * safeCutoff / (float)sampleRate);

        // Feedback amount (resonance)
        float fb = resonance * 4.0f; // 0-4 range for self-oscillation at max

        // Compute feedback with nonlinear saturation
        float feedbackSample = s4;
        float in = input - fb * feedbackSample;

        // Soft clipping on input (analog-style saturation)
        in = std::tanh(in);

        // Guard against NaN from extreme wc values
        if (!std::isfinite(wc) || wc < 0.0f) wc = 0.01f;

        // Four cascaded one-pole filters
        float g = wc / (1.0f + wc);

        float v1 = g * (in - s1);
        float lp1 = v1 + s1;
        s1 = lp1 + v1;

        float v2 = g * (lp1 - s2);
        float lp2 = v2 + s2;
        s2 = lp2 + v2;

        float v3 = g * (lp2 - s3);
        float lp3 = v3 + s3;
        s3 = lp3 + v3;

        float v4 = g * (lp3 - s4);
        float lp4 = v4 + s4;
        s4 = lp4 + v4;

        // Flush denormals from filter state to prevent CPU spikes
        auto flushDenormal = [](float& x) { if (std::abs(x) < 1.0e-15f) x = 0.0f; };
        flushDenormal(s1);
        flushDenormal(s2);
        flushDenormal(s3);
        flushDenormal(s4);

        // NaN recovery: if filter state is corrupted, reset and return silence
        if (!std::isfinite(lp4)) { reset(); return 0.0f; }

        return lp4;
    }

private:
    double sampleRate = 44100.0;
    float cutoff = 10000.0f;
    float resonance = 0.0f;

    // Filter state (4 poles)
    float s1 = 0.0f, s2 = 0.0f, s3 = 0.0f, s4 = 0.0f;
};
