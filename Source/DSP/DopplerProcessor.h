#pragma once

#include <cmath>
#include <vector>
#include <algorithm>

class DopplerProcessor
{
public:
    DopplerProcessor() = default;
    ~DopplerProcessor() = default;

    void prepare(double sampleRate, int maxBlockSize);
    void reset();

    void trigger(int tailDurationSamples);
    void processSampleStereo(float inL, float inR, float& outL, float& outR);

    void setPitchStart(float semitones);
    void setPitchEnd(float semitones);
    bool isActive() const;

private:
    float hermiteInterpolate(float frac, float y0, float y1, float y2, float y3) const;
    float readFromBuffer(int channel, float readPos) const;

    static constexpr int   NUM_CHANNELS       = 2;
    static constexpr int   INTERPOLATION_GUARD = 4;
    static constexpr float MIN_DELAY_SAMPLES   = 2.0f;
    static constexpr float MAX_PITCH_ST        = 24.0f;
    static constexpr float MAX_TAIL_MS         = 5000.0f;
    static constexpr float SWEEP_CURVE         = 3.0f;
    static constexpr float PITCH_THRESHOLD_ST  = 0.05f;
    static constexpr int   INITIAL_DELAY_STEPS = 64;

    double currentSampleRate = 44100.0;
    float pitchStartST = 0.0f;
    float pitchEndST   = 0.0f;

    std::vector<float> delayBuffer[NUM_CHANNELS];
    int bufferSize   = 0;
    int bufferMask   = 0;
    int writeIndex   = 0;

    float currentDelay  = 0.0f;
    float initialDelay  = 0.0f;
    int tailSamples     = 0;
    int sweepIndex      = 0;
    bool sweepActive    = false;
    float curveNorm     = 1.0f;
};
