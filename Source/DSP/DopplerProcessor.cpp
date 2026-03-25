#include "DopplerProcessor.h"

void DopplerProcessor::prepare(double sampleRate, int /*maxBlockSize*/)
{
    currentSampleRate = sampleRate;

    const float maxTailSamples = (MAX_TAIL_MS / 1000.0f) * static_cast<float>(sampleRate);
    const float maxRatio = std::pow(2.0f, MAX_PITCH_ST / 12.0f);
    const int worstCase = static_cast<int>(maxTailSamples * (maxRatio - 1.0f))
                        + INTERPOLATION_GUARD + 1;
    const int totalNeeded = worstCase * 2;

    bufferSize = 1;
    while (bufferSize < totalNeeded + 1)
        bufferSize *= 2;
    bufferMask = bufferSize - 1;

    for (int ch = 0; ch < NUM_CHANNELS; ++ch)
        delayBuffer[ch].resize(static_cast<size_t>(bufferSize), 0.0f);

    curveNorm = 1.0f / (1.0f - std::exp(-SWEEP_CURVE));
    reset();
}

void DopplerProcessor::reset()
{
    for (int ch = 0; ch < NUM_CHANNELS; ++ch)
        std::fill(delayBuffer[ch].begin(), delayBuffer[ch].end(), 0.0f);

    writeIndex = 0;
    currentDelay = MIN_DELAY_SAMPLES;
    initialDelay = MIN_DELAY_SAMPLES;
    sweepIndex = 0;
    sweepActive = false;
}

void DopplerProcessor::trigger(int tailDurationSamples)
{
    tailSamples = tailDurationSamples;
    sweepIndex = 0;
    sweepActive = true;

    float accumDelay = 0.0f;
    float minAccum = 0.0f;

    for (int step = 0; step < INITIAL_DELAY_STEPS; ++step)
    {
        const float progress = static_cast<float>(step) / static_cast<float>(INITIAL_DELAY_STEPS);
        const float curvedP = (1.0f - std::exp(-progress * SWEEP_CURVE)) * curveNorm;
        const float pitchST = pitchStartST + (pitchEndST - pitchStartST) * curvedP;
        const float ratio = std::pow(2.0f, pitchST / 12.0f);
        const float deltaDelay = 1.0f - ratio;
        const float stepSize = static_cast<float>(tailDurationSamples) / static_cast<float>(INITIAL_DELAY_STEPS);
        accumDelay += deltaDelay * stepSize;
        minAccum = std::min(minAccum, accumDelay);
    }

    initialDelay = std::max(MIN_DELAY_SAMPLES, -minAccum + MIN_DELAY_SAMPLES + 4.0f);
    initialDelay = std::min(initialDelay, static_cast<float>(bufferSize / 2));
    currentDelay = initialDelay;
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

        if (sweepIndex >= tailSamples)
        {
            sweepActive = false;
        }
        else
        {
            const float progress = static_cast<float>(sweepIndex) / static_cast<float>(tailSamples);
            const float curvedP = (1.0f - std::exp(-progress * SWEEP_CURVE)) * curveNorm;
            const float pitchST = pitchStartST + (pitchEndST - pitchStartST) * curvedP;
            const float ratio = std::pow(2.0f, pitchST / 12.0f);
            currentDelay += (1.0f - ratio);
            currentDelay = std::max(MIN_DELAY_SAMPLES,
                                    std::min(currentDelay, static_cast<float>(bufferSize / 2)));
        }
    }
    else
    {
        outL = inL;
        outR = inR;
    }

    writeIndex = (writeIndex + 1) & bufferMask;
}

void DopplerProcessor::setPitchStart(float semitones)
{
    pitchStartST = std::clamp(semitones, -MAX_PITCH_ST, MAX_PITCH_ST);
}

void DopplerProcessor::setPitchEnd(float semitones)
{
    pitchEndST = std::clamp(semitones, -MAX_PITCH_ST, MAX_PITCH_ST);
}

bool DopplerProcessor::isActive() const
{
    return std::fabs(pitchStartST) > PITCH_THRESHOLD_ST
        || std::fabs(pitchEndST) > PITCH_THRESHOLD_ST;
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
