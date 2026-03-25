#pragma once

#include <cmath>
#include <algorithm>
#include <array>
#include <juce_core/juce_core.h>
#include "EnvelopeConstants.h"

enum class EnvelopeShape
{
    Exponential = 0,
    Linear,
    Logarithmic,
    ReverseSawtooth,
    DoubleTap,
    Percussive
};

class EnvelopeGenerator
{
public:
    EnvelopeGenerator() = default;
    ~EnvelopeGenerator() = default;

    void prepare(double sampleRate);
    void trigger();
    float getNextSample();
    bool isInTail() const;

    void setTailLength(float ms);
    void setSilenceGap(float ms);
    void setShape(EnvelopeShape shape);
    void setAttackTime(float ms);
    void setHumanize(float percent);
    void setSustainHold(float percent);

    // Set pointer to user-drawn custom curve LUT (owned by caller, must outlive this object)
    void setCustomCurve(const float* lut, int size);

    bool consumeTriggerFlag()
    {
        bool t = justTriggered;
        justTriggered = false;
        return t;
    }

    // Returns normalized position over the full tail+gap cycle (0-1)
    float getNormalizedCyclePosition() const;

    // Static visualization method with hold fraction support.
    // For Custom shape, pass customCurveLUT and customCurveSize.
    static float computeShapeAtNormalized(float normalizedPos, EnvelopeShape shape,
                                          float attackFraction = 0.0f, float holdFraction = 0.0f,
                                          const float* customCurveLUT = nullptr,
                                          int customCurveSize = 0);

private:
    enum class State { Tail, Silence };

    void recalculateSampleCounts();
    void recalculateCoefficients();
    float computeEnvelopeSample() const;

    // State
    State currentState = State::Silence;
    EnvelopeShape currentShape = EnvelopeShape::Exponential;
    double currentSampleRate = 44100.0;

    // Timing (in ms, set by user)
    float tailLengthMs = 150.0f;
    float silenceGapMs = 100.0f;
    float attackTimeMs = 0.1f;
    float sustainHoldPercent = 0.0f;

    // Humanize
    float humanizeAmount = 0.0f;
    juce::Random humanizeRng { 12345 };
    static constexpr float HUMANIZE_VARIATION = 0.20f;

    // Parameter caching
    float cachedTailLengthMs = -1.0f;
    float cachedSilenceGapMs = -1.0f;
    float cachedAttackTimeMs = -1.0f;
    float cachedSustainHoldPercent = -1.0f;

    // Timing (in samples)
    int tailSamples   = 0;
    int gapSamples    = 0;
    int attackSamples = 0;
    int holdSamples   = 0;
    int decaySamples  = 0;
    int sampleIndex   = 0;

    // Pre-computed shape coefficients (based on decaySamples)
    float expDecayRate            = 0.0f;
    float doubleTapSpacingSamples = 0.0f;
    float doubleTapDecayRate      = 0.0f;
    float percBodySamples         = 0.0f;
    float percDecayRate           = 0.0f;

    // Custom curve LUT (not owned — pointer set by TransientEngine)
    const float* customCurveLUT = nullptr;
    int customCurveSize = 0;

    // Trigger flag
    bool justTriggered = false;

    // Crossfade state
    bool crossfading = false;
    int crossfadeIndex = 0;
    float previousEndAmplitude = 0.0f;
};
