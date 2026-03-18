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
    Doppler,
    ReverseSawtooth,
    Gaussian,
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
    void setTension(float t);
    void setHumanize(float percent);
    void setSustainHold(float percent);

    bool consumeTriggerFlag()
    {
        bool t = justTriggered;
        justTriggered = false;
        return t;
    }

    // Static visualization method with hold fraction support.
    static float computeShapeAtNormalized(float normalizedPos, EnvelopeShape shape,
                                          float referenceTailSamples, float tension = 1.0f,
                                          float attackFraction = 0.0f, float holdFraction = 0.0f);

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
    float tension = 1.0f;
    float sustainHoldPercent = 0.0f;

    // Humanize
    float humanizeAmount = 0.0f;
    juce::Random humanizeRng { 12345 };
    static constexpr float HUMANIZE_VARIATION = 0.20f;

    // Parameter caching
    float cachedTailLengthMs = -1.0f;
    float cachedSilenceGapMs = -1.0f;

    // Timing (in samples)
    int tailSamples   = 0;
    int gapSamples    = 0;
    int attackSamples = 0;
    int holdSamples   = 0;
    int decaySamples  = 0;
    int sampleIndex   = 0;

    // Pre-computed shape coefficients (based on decaySamples)
    float expDecayRate            = 0.0f;
    float gaussianSigma           = 1.0f;
    float doubleTapSpacingSamples = 0.0f;
    float doubleTapDecayRate      = 0.0f;
    float percAttackSamples       = 0.0f;
    float percBodySamples         = 0.0f;
    float percDecayRate           = 0.0f;

    // Trigger flag
    bool justTriggered = false;

    // Crossfade state
    bool crossfading = false;
    int crossfadeIndex = 0;
    float previousEndAmplitude = 0.0f;
};
