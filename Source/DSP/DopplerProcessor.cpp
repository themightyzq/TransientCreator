#include "DopplerProcessor.h"

void DopplerProcessor::prepare(double sampleRate, int /*maxBlockSize*/)
{
    currentSampleRate = sampleRate;

    // Pre-allocate buffers for worst case: max tail length at max pitch shift
    const float maxTailSamples = (MAX_TAIL_MS / 1000.0f) * static_cast<float>(sampleRate);
    const float maxGrowthRate = 1.0f - std::pow(2.0f, -MAX_PITCH_SEMITONES / 12.0f);
    const int maxDelay = static_cast<int>(maxTailSamples * maxGrowthRate) + INTERPOLATION_GUARD + 1;

    // Round up to next power of 2 for efficient wrapping
    bufferSize = 1;
    while (bufferSize < maxDelay + 1)
        bufferSize *= 2;

    bufferMask = bufferSize - 1;

    for (int ch = 0; ch < NUM_CHANNELS; ++ch)
        delayBuffer[ch].resize(static_cast<size_t>(bufferSize), 0.0f);

    reset();
}

void DopplerProcessor::reset()
{
    for (int ch = 0; ch < NUM_CHANNELS; ++ch)
        std::fill(delayBuffer[ch].begin(), delayBuffer[ch].end(), 0.0f);

    writeIndex = 0;
    currentDelay = MIN_DELAY_SAMPLES;
    delayIncrement = 0.0f;
    sweepIndex = 0;
    sweepActive = false;
}

void DopplerProcessor::trigger(int tailDurationSamples)
{
    tailSamples = tailDurationSamples;
    sweepIndex = 0;
    currentDelay = MIN_DELAY_SAMPLES;
    sweepActive = true;

    // delayGrowthRate = 1.0 - pow(2.0, -semitones / 12.0)
    const float delayGrowthRate = 1.0f - std::pow(2.0f, -pitchShiftSemitones / 12.0f);

    if (tailSamples > 0)
        delayIncrement = delayGrowthRate;
    else
        delayIncrement = 0.0f;
}

void DopplerProcessor::processSampleStereo(float inL, float inR, float& outL, float& outR)
{
    // Write both channels at the same buffer position
    delayBuffer[0][static_cast<size_t>(writeIndex)] = inL;
    delayBuffer[1][static_cast<size_t>(writeIndex)] = inR;

    if (sweepActive)
    {
        // Compute read position once — shared by both channels
        const float readPos = static_cast<float>(writeIndex) - currentDelay;

        outL = readFromBuffer(0, readPos);
        outR = readFromBuffer(1, readPos);

        // Advance sweep exactly once per sample
        currentDelay += delayIncrement;
        ++sweepIndex;

        if (sweepIndex >= tailSamples)
            sweepActive = false;
    }
    else
    {
        outL = inL;
        outR = inR;
    }

    // Advance write pointer once per sample
    writeIndex = (writeIndex + 1) & bufferMask;
}

float DopplerProcessor::readFromBuffer(int channel, float readPos) const
{
    const int readIndex = static_cast<int>(std::floor(readPos));
    const float frac = readPos - static_cast<float>(readIndex);

    const auto& buf = delayBuffer[channel];
    const float y0 = buf[static_cast<size_t>((readIndex - 1) & bufferMask)];
    const float y1 = buf[static_cast<size_t>((readIndex)     & bufferMask)];
    const float y2 = buf[static_cast<size_t>((readIndex + 1) & bufferMask)];
    const float y3 = buf[static_cast<size_t>((readIndex + 2) & bufferMask)];

    return hermiteInterpolate(frac, y0, y1, y2, y3);
}

void DopplerProcessor::setPitchShiftSemitones(float semitones)
{
    pitchShiftSemitones = semitones;
}

float DopplerProcessor::hermiteInterpolate(float frac, float y0, float y1, float y2, float y3) const
{
    const float c0 = y1;
    const float c1 = 0.5f * (y2 - y0);
    const float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
    const float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);

    return ((c3 * frac + c2) * frac + c1) * frac + c0;
}
