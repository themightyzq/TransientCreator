#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

class DopplerProcessor
{
public:
    DopplerProcessor() = default;
    ~DopplerProcessor() = default;

    void prepare(double sampleRate, int maxBlockSize);
    void process(juce::AudioBuffer<float>& buffer, int numSamples);
    void reset();

    void setPitchShiftSemitones(float semitones);

private:
    double currentSampleRate = 44100.0;
    float pitchShiftSemitones = 12.0f;
};
