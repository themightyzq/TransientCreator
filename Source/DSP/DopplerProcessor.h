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

    // Process a stereo sample pair through the variable delay line.
    // Both channels share the same write index and sweep state —
    // the sweep advances exactly once per call.
    void processSampleStereo(float inL, float inR, float& outL, float& outR);

    void setPitchShiftSemitones(float semitones);

private:
    float hermiteInterpolate(float frac, float y0, float y1, float y2, float y3) const;
    float readFromBuffer(int channel, float readPos) const;

    // DSP constants
    static constexpr int   NUM_CHANNELS        = 2;
    static constexpr int   INTERPOLATION_GUARD  = 4;       // Extra samples for cubic interpolation
    static constexpr float MIN_DELAY_SAMPLES    = 1.0f;    // Minimum delay to avoid zero-delay artifacts
    static constexpr float MAX_PITCH_SEMITONES  = 24.0f;   // Maximum supported pitch shift
    static constexpr float MAX_TAIL_MS          = 5000.0f;  // Maximum tail length

    // State
    double currentSampleRate = 44100.0;
    float pitchShiftSemitones = 12.0f;

    // Per-channel delay buffers (shared write index)
    std::vector<float> delayBuffer[NUM_CHANNELS];
    int bufferSize   = 0;
    int bufferMask   = 0;   // For power-of-2 wrapping
    int writeIndex   = 0;

    // Delay sweep state (shared across channels)
    float currentDelay    = 0.0f;    // Current delay in fractional samples
    float delayIncrement  = 0.0f;    // How much delay grows per sample
    int tailSamples       = 0;       // Duration of current tail
    int sweepIndex        = 0;       // Current sample within the tail sweep
    bool sweepActive      = false;
};
