#include "DopplerProcessor.h"

void DopplerProcessor::prepare(double sampleRate, int /*maxBlockSize*/)
{
    currentSampleRate = sampleRate;

    // Pre-allocate buffer for worst case: max tail length at max pitch shift
    // maxDelay = tailSamples * delayGrowthRate
    // delayGrowthRate = 1.0 - pow(2.0, -maxPitch / 12.0)
    const float maxTailSamples = (MAX_TAIL_MS / 1000.0f) * static_cast<float>(sampleRate);
    const float maxGrowthRate = 1.0f - std::pow(2.0f, -MAX_PITCH_SEMITONES / 12.0f);
    const int maxDelay = static_cast<int>(maxTailSamples * maxGrowthRate) + INTERPOLATION_GUARD + 1;

    // Round up to next power of 2 for efficient wrapping
    bufferSize = 1;
    while (bufferSize < maxDelay + 1)
        bufferSize *= 2;

    bufferMask = bufferSize - 1;
    delayBuffer.resize(static_cast<size_t>(bufferSize), 0.0f);

    reset();
}

void DopplerProcessor::reset()
{
    std::fill(delayBuffer.begin(), delayBuffer.end(), 0.0f);
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

    // Calculate delay growth rate from pitch shift parameter
    // delayGrowthRate = 1.0 - pow(2.0, -semitones / 12.0)
    const float delayGrowthRate = 1.0f - std::pow(2.0f, -pitchShiftSemitones / 12.0f);

    // Total delay accumulation over the tail: maxDelay = tailSamples * delayGrowthRate
    // Per-sample increment to reach that linearly
    if (tailSamples > 0)
        delayIncrement = (static_cast<float>(tailSamples) * delayGrowthRate) / static_cast<float>(tailSamples);
    else
        delayIncrement = 0.0f;

    // Simplifies to just delayGrowthRate, but keeping the derivation explicit
}

float DopplerProcessor::processSample(float input)
{
    // Write input to delay buffer
    delayBuffer[static_cast<size_t>(writeIndex)] = input;

    float output = input;

    if (sweepActive)
    {
        // Read from delay buffer at fractional position
        const float readPos = static_cast<float>(writeIndex) - currentDelay;

        // Get integer and fractional parts
        const int readIndex = static_cast<int>(std::floor(readPos));
        const float frac = readPos - static_cast<float>(readIndex);

        // Fetch 4 samples for Hermite interpolation (y0, y1, y2, y3)
        const float y0 = delayBuffer[static_cast<size_t>((readIndex - 1) & bufferMask)];
        const float y1 = delayBuffer[static_cast<size_t>((readIndex)     & bufferMask)];
        const float y2 = delayBuffer[static_cast<size_t>((readIndex + 1) & bufferMask)];
        const float y3 = delayBuffer[static_cast<size_t>((readIndex + 2) & bufferMask)];

        output = hermiteInterpolate(frac, y0, y1, y2, y3);

        // Advance the delay sweep
        currentDelay += delayIncrement;
        ++sweepIndex;

        if (sweepIndex >= tailSamples)
            sweepActive = false;
    }

    // Advance write pointer
    writeIndex = (writeIndex + 1) & bufferMask;

    return output;
}

void DopplerProcessor::setPitchShiftSemitones(float semitones)
{
    pitchShiftSemitones = semitones;
}

// ---------------------------------------------------------------------------
// Hermite cubic interpolation — 4-point, 3rd-order
// ---------------------------------------------------------------------------

float DopplerProcessor::hermiteInterpolate(float frac, float y0, float y1, float y2, float y3) const
{
    const float c0 = y1;
    const float c1 = 0.5f * (y2 - y0);
    const float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
    const float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);

    return ((c3 * frac + c2) * frac + c1) * frac + c0;
}
