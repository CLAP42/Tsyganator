#pragma once
#include <cmath>
#include <algorithm>
#include <vector>

/**
 * JunoChorus — Stereo chorus effect inspired by the Roland Juno chorus
 * The Juno chorus uses BBD (Bucket-Brigade Device) delay lines modulated by
 * a triangle LFO. Two modes: Chorus I (subtle) and Chorus II (lush).
 * Both combined = the iconic ensemble sound.
 */
class JunoChorus
{
public:
    enum Mode { Off, ChorusI, ChorusII, ChorusI_II };

    void setSampleRate(double sr)
    {
        sampleRate = sr;
        // Max delay ~10ms at 44.1kHz
        int maxDelaySamples = (int)(0.012 * sr);
        delayLineL.resize(maxDelaySamples, 0.0f);
        delayLineR.resize(maxDelaySamples, 0.0f);
        delayLength = maxDelaySamples;
        reset(); // Clear stale audio from previous sample rate
    }

    void setMode(Mode m) { mode = m; }

    void reset()
    {
        std::fill(delayLineL.begin(), delayLineL.end(), 0.0f);
        std::fill(delayLineR.begin(), delayLineR.end(), 0.0f);
        writePos = 0;
        lfoPhase1 = 0.0f;
        lfoPhase2 = 0.25f; // Offset for stereo spread
    }

    // Process stereo (in-place)
    void process(float input, float& outL, float& outR)
    {
        if (mode == Off || delayLength == 0)
        {
            outL = input;
            outR = input;
            return;
        }

        // LFO rates and depths based on mode
        float rate1, rate2, depth;
        switch (mode)
        {
            case ChorusI:
                rate1 = 0.513f;  // Hz
                rate2 = 0.513f;
                depth = 0.0018f; // seconds
                break;
            case ChorusII:
                rate1 = 0.863f;
                rate2 = 0.863f;
                depth = 0.0035f;
                break;
            case ChorusI_II:
                rate1 = 0.513f;
                rate2 = 0.863f;
                depth = 0.0028f;
                break;
            default:
                outL = input;
                outR = input;
                return;
        }

        // Triangle LFO
        float lfo1 = 2.0f * std::abs(2.0f * lfoPhase1 - 1.0f) - 1.0f;
        float lfo2 = 2.0f * std::abs(2.0f * lfoPhase2 - 1.0f) - 1.0f;

        // Delay times in samples
        float centerDelay = 0.005f * (float)sampleRate; // 5ms center
        float modDepth = depth * (float)sampleRate;

        float delayL = centerDelay + lfo1 * modDepth;
        float delayR = centerDelay + lfo2 * modDepth;

        // Write to delay line
        delayLineL[writePos] = input;
        delayLineR[writePos] = input;

        // Read with cubic interpolation
        float wetL = readDelay(delayLineL, delayL);
        float wetR = readDelay(delayLineR, delayR);

        // Mix: 50/50 dry/wet like the original Juno
        outL = 0.5f * (input + wetL);
        outR = 0.5f * (input + wetR);

        // Advance write position
        writePos = (writePos + 1) % delayLength;

        // Advance LFOs (with denormal flushing)
        lfoPhase1 += rate1 / (float)sampleRate;
        if (lfoPhase1 >= 1.0f) lfoPhase1 -= 1.0f;
        if (lfoPhase1 < 1.0e-10f) lfoPhase1 = 0.0f;

        lfoPhase2 += rate2 / (float)sampleRate;
        if (lfoPhase2 >= 1.0f) lfoPhase2 -= 1.0f;
        if (lfoPhase2 < 1.0e-10f) lfoPhase2 = 0.0f;
    }

private:
    double sampleRate = 44100.0;
    Mode mode = Off;

    std::vector<float> delayLineL, delayLineR;
    int delayLength = 0;
    int writePos = 0;

    float lfoPhase1 = 0.0f;
    float lfoPhase2 = 0.25f;

    // Linear interpolation read from delay line (safe bounds)
    float readDelay(const std::vector<float>& line, float delaySamples) const
    {
        if (delayLength <= 0) return 0.0f;

        // Clamp delay to valid range
        delaySamples = std::clamp(delaySamples, 0.0f, (float)(delayLength - 1));

        float readPos = (float)writePos - delaySamples;
        if (readPos < 0.0f) readPos += (float)delayLength;

        int idx0 = (int)readPos;
        if (idx0 >= delayLength) idx0 = delayLength - 1;  // safety clamp
        int idx1 = (idx0 + 1) % delayLength;
        float frac = readPos - (float)idx0;

        return line[idx0] * (1.0f - frac) + line[idx1] * frac;
    }
};
