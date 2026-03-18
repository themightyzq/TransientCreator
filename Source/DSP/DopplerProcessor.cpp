#include "DopplerProcessor.h"

void DopplerProcessor::prepare(double sampleRate, int /*maxBlockSize*/)
{
    currentSampleRate = sampleRate;

    const float maxTailSamples = (MAX_TAIL_MS / 1000.0f) * static_cast<float>(sampleRate);
    const float maxGrowthRate = 1.0f - std::pow(2.0f, -MAX_PITCH_SEMITONES / 12.0f);
    const int maxDelay = static_cast<int>(maxTailSamples * maxGrowthRate) + INTERPOLATION_GUARD + 1;

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
    maxDelayRange = 0.0f;
    curveNormalization = 1.0f;
    sweepIndex = 0;
    sweepActive = false;
}

void DopplerProcessor::trigger(int tailDurationSamples)
{
    tailSamples = tailDurationSamples;
    sweepIndex = 0;
    sweepActive = true;

    const float delayGrowthRate = 1.0f - std::pow(2.0f, -pitchShiftSemitones / 12.0f);
    maxDelayRange = static_cast<float>(tailSamples) * delayGrowthRate;
    curveNormalization = 1.0f / (1.0f - std::exp(-CURVE_SHAPE));

    // Set initial delay based on direction
    currentDelay = computeDelay(0.0f);
}

void DopplerProcessor::processSampleStereo(float inL, float inR, float& outL, float& outR)
{
    delayBuffer[0][static_cast<size_t>(writeIndex)] = inL;
    delayBuffer[1][static_cast<size_t>(writeIndex)] = inR;

    if (sweepActive)
    {
        const float readPos = static_cast<float>(writeIndex) - currentDelay;

        outL = readFromBuffer(0, readPos);
        outR = readFromBuffer(1, readPos);

        ++sweepIndex;
        const float progress = static_cast<float>(sweepIndex) / static_cast<float>(tailSamples);
        currentDelay = computeDelay(progress);

        if (sweepIndex >= tailSamples)
            sweepActive = false;
    }
    else
    {
        outL = inL;
        outR = inR;
    }

    writeIndex = (writeIndex + 1) & bufferMask;
}

void DopplerProcessor::setPitchShiftSemitones(float semitones)
{
    pitchShiftSemitones = semitones;
}

void DopplerProcessor::setDirection(Direction dir)
{
    direction = dir;
}

float DopplerProcessor::computeDelay(float progress) const
{
    auto expCurve = [this](float p) -> float
    {
        return (1.0f - std::exp(-p * CURVE_SHAPE)) * curveNormalization;
    };

    switch (direction)
    {
        case Direction::Recede:
            // Delay increases: fast pitch drop at apex, tapering off
            return MIN_DELAY_SAMPLES + maxDelayRange * expCurve(progress);

        case Direction::Approach:
            // Delay decreases: starts high, pitch rises into apex
            return MIN_DELAY_SAMPLES + maxDelayRange * expCurve(1.0f - progress);

        case Direction::FlyBy:
        {
            // Two halves: approach (delay decreasing) then recede (delay increasing)
            if (progress < 0.5f)
            {
                const float halfProgress = 1.0f - (progress * 2.0f);
                return MIN_DELAY_SAMPLES + maxDelayRange * expCurve(halfProgress);
            }
            else
            {
                const float halfProgress = (progress - 0.5f) * 2.0f;
                return MIN_DELAY_SAMPLES + maxDelayRange * expCurve(halfProgress);
            }
        }

        default:
            return MIN_DELAY_SAMPLES;
    }
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

float DopplerProcessor::hermiteInterpolate(float frac, float y0, float y1, float y2, float y3) const
{
    const float c0 = y1;
    const float c1 = 0.5f * (y2 - y0);
    const float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
    const float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);

    return ((c3 * frac + c2) * frac + c1) * frac + c0;
}
