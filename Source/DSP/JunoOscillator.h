#pragma once
#include <cmath>
#include <algorithm>
#include <random>

/**
 * JunoOscillator — DCO with antialiased Saw & Pulse + Sub oscillator
 * Uses polyBLEP for alias-free waveforms at all frequencies.
 */
class JunoOscillator
{
public:
    void setSampleRate(double sr) { sampleRate = sr; }
    void setFrequency(float freq) { frequency = freq; phaseIncrement = freq / (float)sampleRate; }
    void setPulseWidth(float pw) { pulseWidth = std::clamp(pw, 0.05f, 0.95f); }
    void setSawLevel(float level) { sawLevel = level; }
    void setPulseLevel(float level) { pulseLevel = level; }
    void setTriangleLevel(float level) { triangleLevel = level; }
    void setSubLevel(float level) { subLevel = level; }
    void setNoiseLevel(float level) { noiseLevel = level; }

    void reset()
    {
        phase = 0.0f;
        subPhase = 0.0f;
    }

    // Returns mono sample
    float process()
    {
        float out = 0.0f;

        // --- Saw ---
        if (sawLevel > 0.0f)
        {
            float saw = 2.0f * phase - 1.0f;
            saw -= polyBLEP(phase, phaseIncrement);
            out += saw * sawLevel;
        }

        // --- Pulse (Square) ---
        if (pulseLevel > 0.0f)
        {
            float pulse = (phase < pulseWidth) ? 1.0f : -1.0f;
            pulse += polyBLEP(phase, phaseIncrement);
            pulse -= polyBLEP(fmod(phase - pulseWidth + 1.0f, 1.0f), phaseIncrement);
            out += pulse * pulseLevel;
        }

        // --- Triangle ---
        if (triangleLevel > 0.0f)
        {
            // Naive triangle — aliasing is minimal since harmonics fall off as 1/n²
            float tri = 2.0f * std::abs(2.0f * phase - 1.0f) - 1.0f;
            out += tri * triangleLevel;
        }

        // --- Sub oscillator (square, one octave down) ---
        if (subLevel > 0.0f)
        {
            float subInc = phaseIncrement * 0.5f;
            float sub = (subPhase < 0.5f) ? 1.0f : -1.0f;
            sub += polyBLEP(subPhase, subInc);
            sub -= polyBLEP(fmod(subPhase - 0.5f + 1.0f, 1.0f), subInc);
            out += sub * subLevel;

            subPhase += subInc;
            if (subPhase >= 1.0f) subPhase -= 1.0f;
        }

        // --- White noise ---
        if (noiseLevel > 0.0f)
        {
            out += noiseDist(noiseGen) * noiseLevel;
        }

        // Advance main phase
        phase += phaseIncrement;
        if (phase >= 1.0f) phase -= 1.0f;

        return out;
    }

private:
    double sampleRate = 44100.0;
    float frequency = 440.0f;
    float phase = 0.0f;
    float phaseIncrement = 0.0f;
    float pulseWidth = 0.5f;

    float sawLevel = 1.0f;
    float pulseLevel = 0.0f;
    float triangleLevel = 0.0f;
    float subLevel = 0.0f;
    float noiseLevel = 0.0f;

    // White noise generator
    std::mt19937 noiseGen{ std::random_device{}() };
    std::uniform_real_distribution<float> noiseDist{ -1.0f, 1.0f };

    // Sub oscillator state
    float subPhase = 0.0f;

    // PolyBLEP antialiasing
    static float polyBLEP(float phase, float inc)
    {
        float dt = inc;
        if (dt < 1.0e-8f) return 0.0f; // Guard: zero frequency → no aliasing correction
        if (phase < dt)
        {
            float t = phase / dt;
            return t + t - t * t - 1.0f;
        }
        else if (phase > 1.0f - dt)
        {
            float t = (phase - 1.0f) / dt;
            return t * t + t + t + 1.0f;
        }
        return 0.0f;
    }
};
