#include "DopplerProcessor.h"

void DopplerProcessor::prepare(double sampleRate, int /*maxBlockSize*/)
{
    currentSampleRate = sampleRate;
}

void DopplerProcessor::process(juce::AudioBuffer<float>& /*buffer*/, int /*numSamples*/)
{
    // Stub: passthrough — implemented in Phase 4
}

void DopplerProcessor::reset()
{
    // Stub: reset delay line — implemented in Phase 4
}

void DopplerProcessor::setPitchShiftSemitones(float semitones)
{
    pitchShiftSemitones = semitones;
}
