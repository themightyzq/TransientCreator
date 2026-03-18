#pragma once

#include <cmath>
#include <vector>
#include <algorithm>

class DopplerProcessor
{
public:
    enum class Direction { Recede = 0, Approach, FlyBy };

    DopplerProcessor() = default;
    ~DopplerProcessor() = default;

    void prepare(double sampleRate, int maxBlockSize);
    void reset();

    void trigger(int tailDurationSamples);
    void processSampleStereo(float inL, float inR, float& outL, float& outR);

    void setPitchShiftSemitones(float semitones);
    void setDirection(Direction dir);

private:
    float hermiteInterpolate(float frac, float y0, float y1, float y2, float y3) const;
    float readFromBuffer(int channel, float readPos) const;
    float computeDelay(float progress) const;

    static constexpr int   NUM_CHANNELS        = 2;
    static constexpr int   INTERPOLATION_GUARD  = 4;
    static constexpr float MIN_DELAY_SAMPLES    = 1.0f;
    static constexpr float MAX_PITCH_SEMITONES  = 24.0f;
    static constexpr float MAX_TAIL_MS          = 5000.0f;
    static constexpr float CURVE_SHAPE          = 3.0f;

    double currentSampleRate = 44100.0;
    float pitchShiftSemitones = 12.0f;
    Direction direction = Direction::Recede;

    std::vector<float> delayBuffer[NUM_CHANNELS];
    int bufferSize   = 0;
    int bufferMask   = 0;
    int writeIndex   = 0;

    float currentDelay       = 0.0f;
    float maxDelayRange      = 0.0f;
    float curveNormalization = 1.0f;
    int tailSamples          = 0;
    int sweepIndex           = 0;
    bool sweepActive         = false;
};
