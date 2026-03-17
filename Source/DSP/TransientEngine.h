#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

class TransientEngine
{
public:
    TransientEngine() = default;
    ~TransientEngine() = default;

    void prepare(double sampleRate, int maxBlockSize);
    void processBlock(juce::AudioBuffer<float>& buffer, int numSamples);
    void reset();

private:
    double currentSampleRate = 44100.0;
    int maxBlockSize = 512;
};
