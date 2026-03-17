#include "TransientEngine.h"

void TransientEngine::prepare(double sampleRate, int maxBlock)
{
    currentSampleRate = sampleRate;
    maxBlockSize = maxBlock;
}

void TransientEngine::processBlock(juce::AudioBuffer<float>& /*buffer*/, int /*numSamples*/)
{
    // Stub: passthrough — implemented in Phase 3
}

void TransientEngine::reset()
{
    // Stub: reset state — implemented in Phase 3
}
