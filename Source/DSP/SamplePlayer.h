#pragma once
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <memory>

/**
 * SamplePlayer — Loads and plays back a WAV/AIFF sample
 * Can be triggered by MIDI notes or the sequencer.
 * Supports one-shot or looped playback.
 */
class SamplePlayer
{
public:
    SamplePlayer()
    {
        formatManager.registerBasicFormats(); // WAV, AIFF
    }

    // P2-3: call from prepareToPlay so we can precompute playback ratio.
    void setHostSampleRate(double sr)
    {
        hostSampleRate = (sr > 0.0) ? sr : 44100.0;
        recomputeRatio();
    }

    bool loadFile(const juce::File& file)
    {
        std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));
        if (!reader) return false;

        // CRITICAL-4: Create new buffer separately before swapping
        // Double-buffering approach: avoid pointer swap during audio thread access
        auto newBuffer = std::make_unique<juce::AudioBuffer<float>>();
        newBuffer->setSize((int)reader->numChannels, (int)reader->lengthInSamples);
        reader->read(newBuffer.get(), 0, (int)reader->lengthInSamples, 0, true, true);

        // Atomic swap of the buffer pointer
        {
            juce::SpinLock::ScopedLockType lock(sampleBufferLock);
            sampleBuffer = std::move(*newBuffer);
            fileSampleRate = reader->sampleRate;
            sampleName = file.getFileNameWithoutExtension();
            sampleLoaded = true;
            position = 0;
            isPlaying = false;
            recomputeRatio();
        }
        return true;
    }

    void trigger(float velocity = 1.0f)
    {
        if (!sampleLoaded) return;
        position = 0;
        currentVelocity = velocity;
        isPlaying = true;
    }

    void stop()
    {
        isPlaying = false;
        position = 0;
    }

    void clearSample()
    {
        stop();
        juce::SpinLock::ScopedLockType lock(sampleBufferLock);
        sampleBuffer.setSize(0, 0);
        sampleName = "";
        sampleLoaded = false;
    }

    // Get next sample (mono mix). Call once per audio sample.
    // P2-3: ratio is precomputed in loadFile/setHostSampleRate, no division per sample.
    float process()
    {
        if (!isPlaying || !sampleLoaded)
            return 0.0f;

        // CRITICAL-4: Lock before accessing sampleBuffer to ensure safe read
        juce::SpinLock::ScopedLockType lock(sampleBufferLock);

        if (sampleBuffer.getNumSamples() == 0)
            return 0.0f;

        const double ratio = playbackRatio;
        int intPos = (int)position;
        if (intPos >= sampleBuffer.getNumSamples())
        {
            if (looping)
                position = 0.0;
            else
            {
                isPlaying = false;
                return 0.0f;
            }
            intPos = (int)position;
        }

        // Simple linear interpolation
        float sample = 0.0f;
        int nextPos = intPos + 1;
        if (nextPos >= sampleBuffer.getNumSamples())
            nextPos = looping ? 0 : intPos;

        float frac = (float)(position - intPos);

        for (int ch = 0; ch < sampleBuffer.getNumChannels(); ++ch)
        {
            float s0 = sampleBuffer.getSample(ch, intPos);
            float s1 = sampleBuffer.getSample(ch, nextPos);
            sample += s0 + frac * (s1 - s0);
        }

        // Average channels if stereo
        if (sampleBuffer.getNumChannels() > 1)
            sample *= 0.5f;

        position += ratio;
        return sample * currentVelocity * volume;
    }

    bool hasSample() const { return sampleLoaded; }
    bool getIsPlaying() const { return isPlaying; }
    const juce::String& getSampleName() const { return sampleName; }

    void setVolume(float v) { volume = std::clamp(v, 0.0f, 1.0f); }
    float getVolume() const { return volume; }

    void setLooping(bool l) { looping = l; }
    bool getLooping() const { return looping; }

private:
    void recomputeRatio()
    {
        playbackRatio = (hostSampleRate > 0.0 && fileSampleRate > 0.0)
                      ? (fileSampleRate / hostSampleRate)
                      : 1.0;
    }

    juce::AudioFormatManager formatManager;
    juce::AudioBuffer<float> sampleBuffer;
    mutable juce::SpinLock sampleBufferLock;  // CRITICAL-4: Protects sampleBuffer access
    double fileSampleRate = 44100.0;
    double hostSampleRate = 44100.0;
    double playbackRatio  = 1.0;   // P2-3: precomputed fileSR / hostSR
    juce::String sampleName;
    bool sampleLoaded = false;
    bool isPlaying = false;
    bool looping = false;
    double position = 0.0;
    float currentVelocity = 1.0f;
    float volume = 0.7f;
};
