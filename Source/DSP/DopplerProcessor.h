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

    // Call when a new transient is triggered — resets the delay sweep
    void trigger(int tailDurationSamples);

    // Process a single sample through the variable delay line
    float processSample(float input);

    void setPitchShiftSemitones(float semitones);

private:
    float hermiteInterpolate(float frac, float y0, float y1, float y2, float y3) const;

    // DSP constants
    static constexpr int   INTERPOLATION_GUARD = 4;       // Extra samples for cubic interpolation
    static constexpr float MIN_DELAY_SAMPLES   = 1.0f;    // Minimum delay to avoid zero-delay artifacts
    static constexpr float MAX_PITCH_SEMITONES  = 24.0f;  // Maximum supported pitch shift
    static constexpr float MAX_TAIL_MS          = 5000.0f; // Maximum tail length

    // State
    double currentSampleRate = 44100.0;
    float pitchShiftSemitones = 12.0f;

    // Delay line (circular buffer)
    std::vector<float> delayBuffer;
    int bufferSize   = 0;
    int bufferMask   = 0;   // For power-of-2 wrapping
    int writeIndex   = 0;

    // Delay sweep state
    float currentDelay    = 0.0f;    // Current delay in fractional samples
    float delayIncrement  = 0.0f;    // How much delay grows per sample
    int tailSamples       = 0;       // Duration of current tail
    int sweepIndex        = 0;       // Current sample within the tail sweep
    bool sweepActive      = false;
};
